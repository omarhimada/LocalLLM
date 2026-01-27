#include <LocalLLM.h>

std::string runInference(llama_model* model, const std::string& promptUtf8, const std::function<void(const std::string&)>& onText) {
	llama_context_params params = llama_context_default_params();
	constexpr int maxAppendedTokens = 2048;
	params.n_ctx = 4096; // 2048; //8192;
	params.n_batch = 1024;

	llama_context* ctx = llama_init_from_model(model, params);
	if (!ctx) return "Failed to init llama context.";

	const llama_vocab* vocab = llama_model_get_vocab(model);

	// Tokenize prompt
	std::vector<llama_token> promptTokens = tokenize(vocab, promptUtf8, true, true);

	// Decode prompt to initialize KV-cache + logits
	DecodeResult decodeResult = decodeTokens(ctx, promptTokens, 0);
	if (!decodeResult.ok()) {
		llama_free(ctx);
		std::string failurePrefix = "Failed to decode prompt. Decode result: ";
		std::string failureMessage =
			failurePrefix
			.append(std::to_string(decodeResult.rc))
			.append(" \n Tokens: ")
			.append(std::to_string(promptTokens.size()));

		return failureMessage;
	}

	std::string result = "";  // NOLINT(readability-redundant-string-init)
	int32_t position = static_cast<int32_t>(promptTokens.size());

	// build sampler once per RunInference
	llama_sampler_chain_params scParams = llama_sampler_chain_default_params();
	
	llama_sampler* smpl = llama_sampler_chain_init(scParams);
	
	// anti-loop / quality defaults:
	//llama_sampler_chain_add(smpl, llama_sampler_init_penalties(
	//	/* penalty_last_n */ 128,
	//	/* penalty_repeat */ 1.12f,
	//	/* penalty_freq   */ 0.00f,
	//	/* penalty_pres   */ 0.00f
	//));

	//llama_sampler_chain_add(smpl, llama_sampler_init_top_k(40));
	//llama_sampler_chain_add(smpl, llama_sampler_init_top_p(0.95f, 1));
	//llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.70f));
	//llama_sampler_chain_add(smpl, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

	// Generate
	for (int i = 0; i < maxAppendedTokens; ++i) {
		const float* logits = llama_get_logits(ctx);

		llama_token token = getBestToken(ctx, vocab);

		if (token == llama_vocab_eos(vocab) || token == llama_vocab_eot(vocab))
			break;

		std::string piece = tokenToString(vocab, token);

		// Stream delta
		if (onText) onText(piece);
		result.append(piece);

		// Decode this token so logits update for next step
		std::vector<llama_token> one{ token };

		decodeResult = decodeTokens(ctx, one, position);
		if (!decodeResult.ok()) {
			llama_free(ctx);
			return "Failed to decode result.";
		}

		position += 1;
	}

	llama_free(ctx);
	return result;
}