#include <algorithm>
#include <cstring>
#include <iostream>
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
  if (argc < 1)
    return -1;
  const int num_threads = abs(atoi(argv[1])); // I know this is not very good, but it works
  omp_set_num_threads(num_threads);

  std::cerr << "Reading the molecules from the standard input ..." << std::endl;
  std::unordered_set<char> alphabet_builder;
  std::vector<char> alphabet;
  std::string database;
  database.reserve(209715200);  // 200MB

  std::vector<size_t> permutations(max_pattern_len, 0);

  // declare the dictionary that holds all the ngrams with the greatest coverage
  // of the dictionary
  dictionary result;

  #pragma omp parallel num_threads(2)
  {
    #pragma omp single
    {
      // Read line by line sequentially and fill alphabet_builder and database
      //
      // Trying to parallelize this part would lead to a lot of overhead due to locking
      for (std::string line; std::getline(std::cin, line);) {
        for (const auto& c: line) {
          alphabet_builder.emplace(c);
          database.push_back(c);
        }
      }

      // When we finish with reading from input, we parallely compute the alphabet and the permutations
      #pragma omp task
      {
        alphabet.reserve(alphabet_builder.size());
        for (const auto &c : alphabet_builder)
          alphabet.push_back(c);
      }
      #pragma omp task
      {
        permutations[0] = alphabet_builder.size();
        for (std::size_t i{1}; i < permutations.size(); ++i)
          permutations[i] = alphabet_builder.size() * permutations[i - std::size_t{1}];
      }
    }
  }

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
