#pragma once
#include "llama.h"
#include <string>
#include <vector>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QString>

struct StreamBuffer {
	std::mutex m;
	QString pending;
	bool done = false;
};

struct DecodeResult {
	int rc = 0;
	bool ok() const { return rc == 0; }
	//explicit operator bool() const { return ok(); }
};

/**
 * @brief Tokenizes a text string into a vector of tokens using the specified vocabulary.
 * @param vocab A pointer to the llama vocabulary to use for tokenization.
 * @param text The text string to tokenize.
 * @param add_special Whether to add special tokens during tokenization. Defaults to true.
 * @param parse_special Whether to parse special tokens during tokenization. Defaults to true.
 * @return A vector of llama tokens representing the tokenized text.
 */
std::vector<llama_token> tokenize(const llama_vocab* vocab, const std::string& text, bool add_special, bool parse_special);

/**
 * @brief Converts a token to its string representation.
 * @param vocab A pointer to the vocabulary used for token conversion.
 * @param token The token to convert to a string.
 * @return The string representation of the token, or an empty string if conversion fails.
 */
std::string tokenToString(const llama_vocab* vocab, llama_token token);

/**
 * @brief Finds and returns the token with the highest logit value from the model's output.
 * @param ctx Pointer to the llama context containing the model's current logits.
 * @param vocab Pointer to the vocabulary used to determine the total number of tokens.
 * @return The token ID with the highest logit value (most likely next token).
 */
llama_token getBestToken(llama_context* ctx, const llama_vocab* vocab);

/**
 * @brief Decodes a sequence of tokens using the llama context.
 * @param ctx Pointer to the llama context used for decoding.
 * @param tokens The vector of tokens to decode.
 * @param start_pos The starting position for the token sequence in the context. Defaults to 0.
 * @return True if decoding succeeded, false otherwise.
 */
DecodeResult decodeTokens(llama_context* ctx, const std::vector<llama_token>& tokens, int32_t start_pos);