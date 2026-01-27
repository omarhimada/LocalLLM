#pragma once
#include <LocalLLM.h>

/**
 * @brief Runs inference on a llama model to generate text based on a given prompt.
 * @param model A pointer to the initialized llama model to use for inference.
 * @param promptUtf8 The input prompt string in UTF-8 encoding that seeds the text generation.
 * @param onText
 * @return The generated text string, or an error message if context initialization fails.
 */
std::string runInference(llama_model* model, const std::string& promptUtf8, const std::function<void(const std::string&)>& onText);