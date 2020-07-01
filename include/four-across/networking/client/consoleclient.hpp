#pragma once

#include "four-across/networking/client/client.hpp"

#include <atomic>
#include <queue>
#include <string>
#include <thread>

namespace game
{
	namespace networking
	{
		namespace client

		{
			/*
				Implements a game Client that handles user interaction and commands through the console.
			*/
			class ConsoleClient : public Client
			{
			public:
				ConsoleClient();
			protected:
				virtual void onConnect(uint8_t playerId) override;
				virtual void onDisconnect() override;
				virtual void onQueueUpdate(uint64_t queuePosition) override;
				virtual void onGameStart(uint8_t numPlayers, uint8_t firstPlayer, uint8_t cols, uint8_t rows) override;
				virtual void onGameEnd(uint8_t winner) override;
				virtual void onGameUpdate(uint8_t player, uint8_t col) override;
				virtual void onTurnResult(FourAcross::TurnResult result, uint8_t column) override;

				virtual void handleTurnRequest() override;
			private:
				class InputWorker
				{
				public:
                    using InputCallback = std::function<bool(std::string)>;

					~InputWorker();

                    void start(InputCallback callback);
					void restart(InputCallback callback);
					void stop();

					bool isRunning();
				private:
                    void readUntilDone(InputCallback callback);

					std::thread thread;
					std::atomic<bool> running{false};
				};
				InputWorker inputWorker;
			};
		}
	}
}
