#include <LocalLLM.h>

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) {
	int argc = 0;
	char** argv = nullptr;

	QApplication app(argc, argv);

	/**
	 * Currently troubleshooting MSVC regex incompatibilities/limitations with certain KVs within certain GGUF models.
	 * For example with 'ministral-3-14B-Instruct.gguf'
	 * 
	 * Removed/sanitized tokenizer.chat_template in order to manually apply template due to MSVC regex limitations due to these exceptions when attempting to load the model.
	 * --------------------------------------------
	 * i.e.:
	 *	"Exception thrown at 0x00007FFEEBA1A80A in LocalLLM.exe: Microsoft C++ exception: std::regex_error at memory location 0x000000A358B54910.
	 *	 Exception thrown at 0x00007FFEEBA1A80A in LocalLLM.exe: Microsoft C++ exception: std::runtime_error at memory location 0x000000A358B5E518.
	 *	 Exception thrown at 0x00007FFEEBA1A80A in LocalLLM.exe: Microsoft C++ exception: std::runtime_error at memory location 0x000000A358B5F208.
	 * --------------------------------------------
	 * qwen works
	 */
	const std::wstring modelPathW = getExeDir() + L"\\..\\..\\Models\\qwen.gguf";
	const std::string modelPathUtf8 = wideToUtf8(modelPathW);

	const auto runtime = new LocalRuntime(modelPathUtf8);
	if (runtime->model == nullptr) {
		MessageBoxW(nullptr, L"Failed to load GGUF model.", L"Local LLM", MB_ICONERROR);
		return 1;
	}

	auto window = new QWidget();
	auto* layout = new QVBoxLayout(window);
	
	auto* prompt = new QPlainTextEdit();
	prompt->setFont(makeCodeFont(12));
	prompt->setPlaceholderText("");
	prompt->setMaximumBlockCount(1000);

	// TODO attach to 'enter' key
	auto* sendButton = new QPushButton("⌘");
	//auto* stopButton = new QPushButton("x");
	//sendButton->clicked
	//sendButton->addAction()
	
	// Model output
	auto* output = new QPlainTextEdit();
	output->setFont(makeCodeFont(12));

	auto* debugOutput = new QPlainTextEdit();
	debugOutput->setReadOnly(true);
	output->setPlaceholderText("");

	output->setReadOnly(true);
	output->setPlaceholderText("");

	layout->addWidget(prompt);
	layout->addWidget(sendButton);
	layout->addWidget(output);

	window->resize(812, 812);

	const std::string system = R"(I admire you. You are an excellent software engineer with extensive knowledge of algorithms and ideal optimizations for time amd space complexity. You are an expert with modern C++ and it is your preference. Respond with succinct answers. Do not over-elaborate. Your responses should be short and your code solutions should be elegant.)";
	//const std::string chatTemplate = R"({%- if messages[0]['role'] == 'system' %}
	//										{%- set system_message = messages[0]['content'] %}
	//										{%- set loop_messages = messages[1:] %}
	//									{%- else %}
	//										{%- set loop_messages = messages %}
	//									{%- endif %}
	//									{{- bos_token }}
	//									{%- for message in loop_messages %}
	//									{%- if (message['role'] == 'user') != (loop.index0 % 2 == 0) %}
	//									{{- raise_exception('After the optional system message, conversation roles must alternate user/assistant/user/assistant/...') }}
	//									{%- endif %}
	//									{%- if message['role'] == 'user' %}
	//									{%- if loop.first and system_message is defined %}
	//									{{- '[INST] ' + system_message + '\n\n' + message['content'] + ' [/INST]' }}
	//									{%- else %}
	//									{{- '[INST] ' + message['content'] + ' [/INST]' }}
	//									{%- endif %}
	//									{%- elif message['role'] == 'assistant' %}
	//									{{- ' ' + message['content'] + eos_token }}
	//									{%- else %}
	//									{{- raise_exception('Only user and assistant roles are supported, with the exception of an initial optional system message!') }}
	//									{%- endif %}
	//									{%- endfor %}
	//									{%- if add_generation_prompt %}{{- ' ' }}{%- endif %})";

	const std::string chatTemplateQwen3 = R"({ % -if tools% }
									{ { -'<|im_start|>system\n' } }
									{ % -if messages[0].role == 'system'% }
									{ { -messages[0].content + '\n\n' } }
									{ % -endif% }
									{ { -"# Tools\n\nYou may call one or more functions to assist with the user query.\n\nYou are provided with function signatures within <tools></tools> XML tags:\n<tools>" } }
									{ % -for tool in tools% }
									{ { -"\n" } }
									{ { -tool | tojson } }
									{ % -endfor% }
									{ { -"\n</tools>\n\nFor each function call, return a json object with function name and arguments within <tool_call></tool_call> XML tags:\n<tool_call>\n{\"name\": <function-name>, \"arguments\": <args-json-object>}\n</tool_call><|im_end|>\n" } }
									{ % -else% }
									{ % -if messages[0].role == 'system'% }
									{ { -'<|im_start|>system\n' + messages[0].content + '<|im_end|>\n' } }
									{ % -endif% }
									{ % -endif% }
									{ % -set ns = namespace(multi_step_tool = true, last_query_index = messages | length - 1)% }
									{ % -for forward_message in messages% }
									{ % -set index = (messages | length - 1) - loop.index0% }
									{ % -set message = messages[index] % }
									{ % -set current_content = message.content if message.content is not none else ''% }
									{ % -set tool_start = '<tool_response>'% }
									{ % -set tool_start_length = tool_start | length% }
									{ % -set start_of_message = current_content[:tool_start_length] % }
									{ % -set tool_end = '</tool_response>'% }
									{ % -set tool_end_length = tool_end | length% }
									{ % -set start_pos = (current_content | length) - tool_end_length% }
									{ % -if start_pos < 0 % }
									{ % -set start_pos = 0 % }
									{ % -endif% }
									{ % -set end_of_message = current_content[start_pos:] % }
									{ % -if ns.multi_step_tool and message.role == "user" and not(start_of_message == tool_start and end_of_message == tool_end)% }
									{ % -set ns.multi_step_tool = false % }
									{ % -set ns.last_query_index = index% }
									{ % -endif% }
									{ % -endfor% }
									{ % -for message in messages% }
									{ % -if (message.role == "user") or (message.role == "system" and not loop.first)% }
									{ { -'<|im_start|>' + message.role + '\n' + message.content + '<|im_end|>' + '\n' } }
									{ % -elif message.role == "assistant"% }
									{ % -set content = message.content% }
									{ % -set reasoning_content = ''% }
									{ % -if message.reasoning_content is defined and message.reasoning_content is not none% }
									{ % -set reasoning_content = message.reasoning_content% }
									{ % -else% }
									{ % -if '</think>' in message.content% }
									{ % -set content = (message.content.split('</think>') | last).lstrip('\n')% }
									{ % -set reasoning_content = (message.content.split('</think>') | first).rstrip('\n')% }
									{ % -set reasoning_content = (reasoning_content.split('<think>') | last).lstrip('\n')% }
									{ % -endif% }
									{ % -endif% }
									{ % -if loop.index0 > ns.last_query_index% }
									{ % -if loop.last or (not loop.last and reasoning_content)% }
									{ { -'<|im_start|>' + message.role + '\n<think>\n' + reasoning_content.strip('\n') + '\n</think>\n\n' + content.lstrip('\n') } }
									{ % -else% }
									{ { -'<|im_start|>' + message.role + '\n' + content } }
									{ % -endif% }
									{ % -else% }
									{ { -'<|im_start|>' + message.role + '\n' + content } }
									{ % -endif% }
									{ % -if message.tool_calls% }
									{ % -for tool_call in message.tool_calls% }
									{ % -if (loop.first and content) or (not loop.first)% }
									{ { -'\n' } }
									{ % -endif% }
									{ % -if tool_call.function% }
									{ % -set tool_call = tool_call.function% }
									{ % -endif% }
									{ { -'<tool_call>\n{"name": "' } }
									{ { -tool_call.name } }
									{ { -'", "arguments": ' } }
									{ % -if tool_call.arguments is string% }
									{ { -tool_call.arguments } }
									{ % -else% }
									{ { -tool_call.arguments | tojson } }
									{ % -endif% }
									{ { -'}\n</tool_call>' } }
									{ % -endfor% }
									{ % -endif% }
									{ { -'<|im_end|>\n' } }
									{ % -elif message.role == "tool"% }
									{ % -if loop.first or (messages[loop.index0 - 1].role != "tool")% }
									{ { -'<|im_start|>user' } }
									{ % -endif% }
									{ { -'\n<tool_response>\n' } }
									{ { -message.content } }
									{ { -'\n</tool_response>' } }
									{ % -if loop.last or (messages[loop.index0 + 1].role != "tool")% }
									{ { -'<|im_end|>\n' } }
									{ % -endif% }
									{ % -endif% }
									{ % -endfor% }
									{ % -if add_generation_prompt% }
									{ { -'<|im_start|>assistant\n' } }
									{ % -if enable_thinking is defined and enable_thinking is false % }
									{ { -'<think>\n\n</think>\n\n' } }
									{ % -endif% }
									{ % -endif% })";

	QObject::connect(sendButton, &QPushButton::clicked, [&]() {
		// Prevent double-click
		sendButton->setEnabled(false);

		output->clear();

		const QString qPrompt = prompt->toPlainText();
		
		if (qPrompt.isEmpty())
			return;

		const std::string promptUtf8 = qPrompt.toUtf8().toStdString();

		const std::string usr = qPrompt.toStdString();

		const llama_chat_message messages[] = {
		   { "system", system.c_str() },
		   { "user", usr.c_str() }
		};

		std::string formatted;

		const int requiredSize = llama_chat_apply_template(
			chatTemplateQwen3.c_str(),
			messages,
			std::size(messages),
			true,
			nullptr,
			0
		);

		// NOLINT(bugprone-implicit-widening-of-multiplication-result)
		formatted.resize(static_cast<size_t>(requiredSize) + 1);

		int written = llama_chat_apply_template(
			chatTemplateQwen3.c_str(),
			messages,
			std::size(messages),
			true,
			formatted.data(),
			static_cast<int>(formatted.size())
		);

		if (written < 0) {
			throw std::invalid_argument("Prompt template could not apply.");
		}

		output->appendPlainText("");

		auto streamBuffer = std::make_shared<StreamBuffer>();
		auto uiTimer = new QTimer(window);
		uiTimer->setInterval(15); // ~60 fps

		QObject::connect(uiTimer, &QTimer::timeout, [window, output, streamBuffer, uiTimer]() {
			QString chunk;
			{
				std::scoped_lock lock(streamBuffer->m);
				if (streamBuffer->pending.isEmpty() && !streamBuffer->done) return;
				chunk.swap(streamBuffer->pending);
				if (streamBuffer->done && chunk.isEmpty()) {
					uiTimer->stop(); auto* logEdit = new QPlainTextEdit();
					logEdit->setReadOnly(true);
					logEdit->setMaximumBlockCount(2000); // keeps memory stable

					auto* dock = new QDockWidget("Runtime");
					dock->setWidget(logEdit);
					dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

					//window->a(Qt::BottomDockWidgetArea, dock);

					// Helper
					auto log = [&](const QString& s) {
						logEdit->appendPlainText(QTime::currentTime().toString("HH:mm:ss") + "  " + s);
					};
				}
			}
			output->moveCursor(QTextCursor::End);
			output->insertPlainText(chunk);
		});

		uiTimer->start();

		// Worker callback (called from inference thread)
		std::function onText = [streamBuffer](const std::string& piece) {
			const QString q = QString::fromUtf8(piece.c_str());
			std::scoped_lock lock(streamBuffer->m);
			streamBuffer->pending += q;
		};

		QPlainTextEdit* outputPtr = output;

		// Run inference off the UI thread
		QFuture future = QtConcurrent::run([model = runtime->model, promptUtf8, onText, streamBuffer, outputPtr, sendButton]() mutable {
			// Run inference (streams via onText -> streamBuffer)

			const std::string finalOrError = runInference(model, promptUtf8, onText);

			// Timer stops when buffer drains
			{
				std::scoped_lock lock(streamBuffer->m);
				streamBuffer->done = true;
			}

			// If RunInference returned a non-empty error string, show it (optional)
			if (!finalOrError.empty()) {
				QMetaObject::invokeMethod(
					outputPtr,
					[outputPtr, msg = QString::fromUtf8(finalOrError.c_str())]() {
						outputPtr->appendPlainText("\n" + msg);
					},
					Qt::QueuedConnection
				);
			}

			// Re-enable button on UI thread
			QMetaObject::invokeMethod(
				sendButton,
				[sendButton]() { sendButton->setEnabled(true); },
				Qt::QueuedConnection
			);
		});
	});

	window->show();

	return QApplication::exec();
}
