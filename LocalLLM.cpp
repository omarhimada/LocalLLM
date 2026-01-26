#include "LocalLLM.h"

/**
 * @brief Entry point for the LocalLLM Qt application. Initializes the Qt framework, loads a GGUF language model, creates the main GUI window with prompt input and output display areas, and starts the application event loop.
 * @params  WinMain parameters: instance handle, previous instance handle (unused), command line arguments, and show command. These parameters are not directly used by the function.
 * @return The application exit code returned by the Qt event loop, or 1 if the model fails to load.
 */
int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int) {
	int argc = 0;
	char** argv = nullptr;

	QApplication app(argc, argv);

	// Qwen3-VL-8B-Thinking
	std::wstring modelPathW = getExeDir() + L"\\..\\..\\Models\\qwen.gguf";
	const std::string modelPathUtf8 = wideToUtf8(modelPathW);

	LocalRuntime* runtime = new LocalRuntime(modelPathUtf8);
	if (runtime->model == nullptr) {
		MessageBoxW(nullptr, L"Failed to load GGUF model.", L"Local LLM", MB_ICONERROR);
		return 1;
	}

	QWidget* window = new QWidget();
	auto* layout = new QVBoxLayout(window);
	
	auto* prompt = new QPlainTextEdit();
	prompt->setPlaceholderText("");
	prompt->setMaximumBlockCount(1000);

	// TODO attach to 'enter' key
	auto* sendButton = new QPushButton("⌘");
	//auto* stopButton = new QPushButton("x");
	//sendButton->clicked
	//sendButton->addAction()
	
	// Model output
	auto* output = new QPlainTextEdit();

	auto* debugOutput = new QPlainTextEdit();
	debugOutput->setReadOnly(true);
	output->setPlaceholderText("");

	output->setReadOnly(true);
	output->setPlaceholderText("");

	layout->addWidget(prompt);
	layout->addWidget(sendButton);
	layout->addWidget(output);
	
	/**
	 * @brief Handles the Send button click by running LLM inference off the UI thread and streaming tokens into the output box.
	 * @details Clears prior output, validates the prompt, then starts a UI timer that periodically flushes a thread-safe
	 *          buffer of streamed token text into the `output` editor. Inference runs via `QtConcurrent::run` and reports
	 *          token pieces through a callback that appends to the shared `StreamBuffer`. The Send button is disabled
	 *          during generation to prevent re-entrancy, and re-enabled when generation completes. Any non-empty return
	 *          from `RunInference` is treated as an error message and appended to the output.
	 * @param None.
	 * @return None.
	 */
	QObject::connect(sendButton, &QPushButton::clicked, [&]() {
		output->clear();

		const QString qPrompt = prompt->toPlainText();
		if (qPrompt.isEmpty())
			return;

		const std::string promptUtf8 = qPrompt.toUtf8().toStdString();
		llama_model* model = runtime->model;

		// Note: default template is a lot more complex for something we're trying to debug with.
		//const char* tmpl = llama_model_chat_template(model, nullptr);
		const char* qwen3VlJinjaTemplate = R"JINJA(
			{{- if .system -}}{{ .system }} {{- end -}}{{ .prompt }}<end_of_turn>
			{% -for message in messages%} 
			{{ -'<|im_start|>' + message['role'] + '\n' }} 
			{% -if message['role'] == 'assistant' and 'reasoning_content' in message% } 
			{{ -'<think>\n' + message['reasoning_content'] + '\n</think>\n' }} 
			{% -endif% } 
			{{ -message['content'] + '<|im_end|>\n' }} 
			{% -endfor% } 
		)JINJA";

		/*{ % -if add_generation_prompt% } \
		{{ -'<|im_start|>assistant\n<think>\n' }}
		{ % -endif% }*/

		const std::string sys = "You are an excellent software engineer with extensive knowledge of algorithms and ideal optimizing for time amd space complexity. You are familiar with modern C++.";
		const std::string usr = qPrompt.toStdString();

		llama_chat_message messages[] = {
		   { "system", sys.c_str() },
		   { "user", usr.c_str() }
		};

		std::string formatted;
		formatted.resize(32 * 1024);

		const int n = llama_chat_apply_template(
			qwen3VlJinjaTemplate,
			messages,
			std::size(messages),
			true,
			formatted.data(),
			static_cast<int>(formatted.size())
		);

		if (n < 0) {
			throw std::invalid_argument("Prompt template could not apply.");
		}

		output->appendPlainText(""); // Thinking ... (placeholder)

		auto streamBuffer = std::make_shared<StreamBuffer>();
		QTimer* _uiTimer = new QTimer(window);
		_uiTimer->setInterval(16); // ~60 fps

		QObject::connect(_uiTimer, &QTimer::timeout, [window, output, streamBuffer, _uiTimer]() {
			QString chunk;
			{
				std::scoped_lock lock(streamBuffer->m);
				if (streamBuffer->pending.isEmpty() && !streamBuffer->done) return;
				chunk.swap(streamBuffer->pending);
				if (streamBuffer->done && chunk.isEmpty()) {
					_uiTimer->stop(); auto* logEdit = new QPlainTextEdit();
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

		_uiTimer->start();

		// Worker callback (called from inference thread)
		std::function onText = [streamBuffer](const std::string& piece) {
			const QString q = QString::fromUtf8(piece.c_str());
			std::scoped_lock lock(streamBuffer->m);
			streamBuffer->pending += q;
		};

		// Prevent double-click
		sendButton->setEnabled(false);

		QPlainTextEdit* outputPtr = output;

		// Run inference off the UI thread
		QFuture future = QtConcurrent::run([model = runtime->model, promptUtf8, onText, streamBuffer, outputPtr, sendButton]() mutable {
			// Run inference (streams via onText -> streamBuffer)
			std::string finalOrError = runInference(model, promptUtf8, onText);

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

	window->resize(812, 812);
	window->show();

	return QApplication::exec();
}
