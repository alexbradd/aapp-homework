#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unordered_set>
#include <vector>

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

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
  // read the whole database of SMILES and put them in a single string
  // NOTE: we can figure out which is our alphabet
  std::cerr << "Reading the molecules from the standard input ..." << std::endl;
  std::unordered_set<char> alphabet_builder;
  std::string database;
  database.reserve(209715200);  // 200MB
  for (std::string line; std::getline(std::cin, line);
       /* automatically handled */) {
    for (const auto character : line) {
      alphabet_builder.emplace(character);
      database.push_back(character);
    }
  }

  // put the alphabet in a container with random access capabilities
  std::vector<char> alphabet;
  alphabet_builder.reserve(alphabet_builder.size());
  std::for_each(std::begin(alphabet_builder), std::end(alphabet_builder),
                [&alphabet](const auto character) { alphabet.push_back(character); });

  // precompute the number of permutations according to the number of characters
  auto permutations = std::vector(max_pattern_len, alphabet.size());
  for (std::size_t i{1}; i < permutations.size(); ++i) {
    permutations[i] = alphabet.size() * permutations[i - std::size_t{1}];
  }

  // declare the dictionary that holds all the ngrams with the greatest coverage
  // of the dictionary
  dictionary result;

  // this outer loop goes through the n-gram with different sizes
  for (std::size_t ngram_size{1}; ngram_size <= max_pattern_len; ++ngram_size) {
    std::cerr << "Evaluating ngrams with " << ngram_size << " characters" << std::endl;

    // this loop goes through all the permutation of the current ngram-size
    const auto num_words = permutations[ngram_size - std::size_t{1}];
    for (std::size_t word_index{0}; word_index < num_words; ++word_index) {
      // compose the ngram
      word current_word;
      memset(current_word.ngram, '\0', max_pattern_len + 1);
      for (std::size_t character_index{0}, remaining_size = word_index; character_index < ngram_size;
           ++character_index, remaining_size /= alphabet.size()) {
        current_word.ngram[character_index] = alphabet[remaining_size % alphabet.size()];
      }
      current_word.size = ngram_size;

      // evaluate the coverage and add the word to the dictionary
      current_word.coverage = count_coverage(database, current_word.ngram);
      result.add_word(current_word);
    }

    // dump an intermediate version after computing a certain number of
    // characters
    std::cerr << "Current dictionary:" << std::endl;
    result.write(std::cerr);
  }

  // generate the final dictionary
  // NOTE: we sort it for pretty-printing
  std::cout << "NGRAM COVERAGE" << std::endl;
  std::sort(std::begin(result.data), std::end(result.data), word_coverage_gt_comparator{});
  result.write(std::cout);
  return EXIT_SUCCESS;
}
