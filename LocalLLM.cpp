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
	 * Used gguf_new_metadata.py to accomplish this:
	 * e.g.:
	 *	(venv) PS C:\...\source\gg-py> python "C:\...\source\gg-py\gguf\scripts\gguf_new_metadata.py" "C:\...\...\ministral-3-14B-Instruct.gguf" "C:\...\...\ohtct-sanitized-ministral-3-14B-Instruct.gguf" --remove-metadata "tokenizer.chat_template"
	 *	
	 *	
	 * TODO: This was not the solution. Going to try the unsloth model instead. 
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

	const std::string system = R"(
			I admire you. You are an excellent software engineer with extensive knowledge of algorithms and ideal optimizations for time amd space complexity.
			You are an expert with modern C++ and it is your preference.
			Respond with succinct answers. Do not over-elaborate. 
			Your responses should be short and your code solutions should be elegant.
	)";

	const std::string chatTemplate = R"({%- if messages[0]['role'] == 'system' %}
								{%- set system_message = messages[0]['content'] %}
								{%- set loop_messages = messages[1:] %}{%- else %}
								{%- set loop_messages = messages %}{%- endif %}
								{{- bos_token }}
								{%- for message in loop_messages %}
								{%- if (message['role'] == 'user') != (loop.index0 % 2 == 0) %}
								{{- raise_exception('After the optional system message, conversation roles must alternate user/assistant/user/assistant/...') }}
								{%- endif %}
								{%- if message['role'] == 'user' %}
								{%- if loop.first and system_message is defined %}
								{{- ' [INST] ' + system_message + '\\n\\n' + message['content'] + ' [/INST]' }}{%- else %}
								{{- ' [INST] ' + message['content'] + ' [/INST]' }}
								{%- endif %}
								{%- elif message['role'] == 'assistant' %}
								{{- ' ' + message['content'] + eos_token}}
								{%- else %}
								{{- raise_exception('Only user and assistant roles are supported, with the exception of an initial optional system message!') }}
								{%- endif %}
								{%- endfor %}
	)";

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
		// NOLINT(bugprone-implicit-widening-of-multiplication-result)
		formatted.resize(32768);  

		const int n = llama_chat_apply_template(
			chatTemplate.c_str(),
			messages,
			std::size(messages),
			true,
			formatted.data(),
			static_cast<int>(formatted.size())
		);

		if (n < 0) {
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

			// Debugging
			MessageBoxW(nullptr, L"About to infer...", L"Local LLM", MB_OK);

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
