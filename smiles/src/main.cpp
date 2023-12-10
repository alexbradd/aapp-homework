#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <omp.h>

#include "mpi_error_check.hpp"

// set the maximum size of the ngram
static constexpr size_t max_pattern_len = 3;
static constexpr size_t max_dictionary_size = 128;
static_assert(max_pattern_len > 1, "The pattern must contain at least one character");
static_assert(max_dictionary_size > 1, "The dictionary must contain at least one element");

// simple class to represent a word of our dictionary
struct word {
  char ngram[max_pattern_len + 1];  // the string data, statically allocated
  size_t size = 0;                  // the string size
  size_t coverage = 0;              // the score of the word
};

// utility struct to ease working with words
struct word_coverage_lt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage < w2.coverage; }
};
struct word_coverage_gt_comparator {
  bool operator()(const word &w1, const word &w2) const { return w1.coverage > w2.coverage; }
};

// this is our dictionary of ngram
struct dictionary {
  std::vector<word> data;
  std::vector<word>::iterator worst_element;

  void add_word(word &new_word) {
    const auto coverage = new_word.coverage;
    if (data.size() < max_dictionary_size) {
      data.emplace_back(std::move(new_word));
      worst_element = std::end(data);
    } else {
      if (worst_element == std::end(data)) {
        worst_element = std::min_element(std::begin(data), std::end(data), word_coverage_lt_comparator{});
      }
      if (coverage > worst_element->coverage) {
        *worst_element = std::move(new_word);
        worst_element = std::end(data);
      }
    }
  }

  void write(std::ostream &out) const {
    for (const auto &word : data) {
      out << word.ngram << ' ' << word.coverage << std::endl;
    }
    out << std::flush;
  }
};

size_t count_coverage(const std::string &dataset, const char *ngram) {
  size_t index = 0;
  size_t counter = 0;
  const size_t ngram_size = ::strlen(ngram);
  while (index < dataset.size()) {
    index = dataset.find(ngram, index);
    if (index != std::string::npos) {
      ++counter;
      index += ngram_size;
    }
  }
  return counter * ngram_size;
}

// OpenMP number of threads is passed via environment variable by launch.sh
int main(int argc, char *argv[]) {
  if (argc < 3)
    return EXIT_FAILURE;
  const int num_threads = abs(atoi(argv[1])); // I know this is not very good, but it works
  omp_set_num_threads(num_threads);
  const std::string filename(argv[2]);

  // ==== Allocate needed structures
  std::vector<char> alphabet;
  std::string database;
  database.reserve(209715200);  // 200MB

  std::vector<size_t> permutations(max_pattern_len, 0);

  // declare the dictionary that holds all the ngrams with the greatest coverage
  // of the dictionary
  dictionary result;

  // ==== Input
  std::cerr << "Reading the molecules..." << std::endl;
  std::ifstream in_file(filename, std::ios::ate);
  // that the file exists is checked by the launcher script
  size_t filesize = static_cast<size_t>(in_file.tellg());
  in_file.seekg(0);

  // Parallely read at different offsets
  std::vector<std::string> reads(num_threads + 1);
  std::vector<std::unordered_set<char>> builders(num_threads + 1);
  #pragma omp parallel default(none) shared(filename, filesize, reads, builders)
  {
    std::ifstream file(filename);
    size_t chunk = filesize / omp_get_num_threads();
    size_t initial_off = omp_get_thread_num() * chunk;
    file.seekg(initial_off);

    char c;
    for (size_t i = initial_off; i < initial_off + chunk; i++) {
      file >> std::noskipws >> c;
      if (c != '\n' && c != '\r' && c != ' ' && c != '\t') {
        builders[omp_get_thread_num()].emplace(c);
        reads[omp_get_thread_num()].push_back(c);
      }
    }
  }
  // Read what is left
  in_file.seekg(-(filesize % num_threads), std::ios::end);
  char c;
  do {
    in_file >> c;
    if (in_file.eof()) break;
    reads[num_threads].push_back(c);
    builders[num_threads].emplace(c);
  } while (!in_file.eof());
  // Reduce parallely into database and alphabet
  #pragma omp parallel default(none) shared(reads, builders, database, alphabet)
  {
    #pragma omp single nowait
    {
      #pragma omp task
      {
        for (const auto &r: reads)
          database.append(r);
      }
      #pragma omp task
      {
        std::unordered_set<char> alphabet_builder;
        for (const auto &b: builders)
          for (const auto &c: b)
            alphabet_builder.emplace(c);
        alphabet.reserve(alphabet_builder.size());
        for (const auto &c: alphabet_builder)
          alphabet.push_back(c);
      }
    }
  }

  permutations[0] = alphabet.size();
  for (std::size_t i{1}; i < permutations.size(); ++i)
    permutations[i] = alphabet.size() * permutations[i - std::size_t{1}];

  // ==== algorithm
  // this outer loop goes through the n-gram with different sizes
  for (std::size_t ngram_size{1}; ngram_size <= max_pattern_len; ++ngram_size) {
    std::cerr << "Evaluating ngrams with " << ngram_size << " characters" << std::endl;

    // this loop goes through all the permutation of the current ngram-size
    const auto num_words = permutations[ngram_size - std::size_t{1}];
    std::vector<word> words(num_words);

    #pragma omp parallel for schedule(dynamic)
    for (std::size_t word_index = 0; word_index < num_words; ++word_index) {
      // compose the ngram
      word& current_word = words[word_index];
      memset(current_word.ngram, '\0', max_pattern_len + 1);
      for (std::size_t character_index{0}, remaining_size = word_index; character_index < ngram_size;
           ++character_index, remaining_size /= alphabet.size()) {
        current_word.ngram[character_index] = alphabet[remaining_size % alphabet.size()];
      }
      current_word.size = ngram_size;

      // evaluate the coverage and add the word to the dictionary
      current_word.coverage = count_coverage(database, current_word.ngram);
    }

    for (auto &w: words)
      result.add_word(w);
  }

  // generate the final dictionary
  std::cout << "NGRAM COVERAGE" << std::endl;
  std::sort(std::begin(result.data), std::end(result.data), word_coverage_gt_comparator{});
  result.write(std::cout);
  return EXIT_SUCCESS;
}
