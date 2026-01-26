#include "LocalLLM.Tokenization.h"

std::string tokenToString(
	const llama_vocab* vocab,
	llama_token token) {
	std::string out;
	out.resize(64);

	while (true) {
		int32_t n = llama_token_to_piece(
			vocab,
			token,
			out.data(),
			static_cast<int32_t>(out.size()),
			/* lstrip */ 0,
			/* special */ true
		);

		if (n <= 0) {
			// Some builds return 0/negative on failure or insufficient buffer.
			// If negative means "required size", handle it:
			int32_t need = -n;
			if (need > 0) {
				out.resize(static_cast<size_t>(need));
				continue;
			}
			return {};
		}

		if (n <= static_cast<int32_t>(out.size())) {
			out.resize(static_cast<size_t>(n));
			return out;
		}

		// If n is a required size hint (some builds do this), grow and retry.
		out.resize(static_cast<size_t>(n));
	}
}

llama_token getBestToken(
	llama_context* ctx,
	const llama_vocab* vocab) {
	const float* logits = llama_get_logits(ctx);
	const int32_t n_vocab = llama_vocab_n_tokens(vocab);

	int best = 0;
	float bestLogit = logits[0];

	for (int32_t i = 1; i < n_vocab; ++i) {
		if (logits[i] > bestLogit) {
			bestLogit = logits[i];
			best = (int)i;
		}
	}

	return (llama_token)best;
}

DecodeResult decodeTokens(
	llama_context* ctx,
	const std::vector<llama_token>& tokens,
	int32_t start_pos = 0) {
	llama_batch batch = llama_batch_init(static_cast<int32_t>(tokens.size()), 0, 1);
	batch.n_tokens = static_cast<int32_t>(tokens.size());

	for (int32_t i = 0; i < static_cast<int32_t>(tokens.size()); ++i) {
		batch.token[i] = tokens[i];
		batch.pos[i] = start_pos + i;
		batch.n_seq_id[i] = 1;
		batch.seq_id[i][0] = 0;

		// Only request logits for the last token in the batch
		batch.logits[i] = (i == static_cast<int32_t>(tokens.size()) - 1);
	}

	const int rc = llama_decode(ctx, batch);
	llama_batch_free(batch); 

	// success return 0;   
	// failure return 1;
	return DecodeResult(rc);
}

std::vector<llama_token> tokenize(
	const llama_vocab* vocab,
	const std::string& text,
	bool add_special = true,
	bool parse_special = true) {
	// First pass: get required token count
	int32_t n = llama_tokenize(
		vocab,
		text.c_str(),
		static_cast<int32_t>(text.size()),
		nullptr,
		0,
		add_special,
		parse_special
	);

	if (n < 0)
		n = -n;

	std::vector<llama_token> tokens(n);

	// Second pass: actual tokenization
	int32_t n2 = llama_tokenize(
		vocab,
		text.c_str(),
		static_cast<int32_t>(text.size()),
		tokens.data(),
		static_cast<int32_t>(tokens.size()),
		add_special,
		parse_special
	);

	if (n2 < 0)
		n2 = -n2;

	tokens.resize(n2);
	return tokens;
}