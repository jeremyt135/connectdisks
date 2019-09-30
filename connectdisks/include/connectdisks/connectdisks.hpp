#pragma once

#include "connectdisks/board.hpp"

namespace connectdisks
{
	class ConnectDisks
	{
	public:
		ConnectDisks(Board::player_t numPlayers = {2},
			Board::board_size_t numColumns = Board::minColumns, 
			Board::board_size_t numRows = Board::minRows);
		ConnectDisks(ConnectDisks&& connect) noexcept;
		~ConnectDisks();

		enum class TurnResult : uint8_t;
		TurnResult takeTurn(Board::player_t player, Board::board_size_t column);

		inline bool hasWinner() const noexcept; 

		inline Board::player_t getWinner() const noexcept;

		inline Board::player_t getCurrentPlayer() const noexcept;
		inline Board::player_t getNumPlayers() const noexcept;
		inline uint32_t getNumTurns() const noexcept;

		ConnectDisks& operator=(ConnectDisks&& connect) noexcept;

		static constexpr Board::player_t noWinner{Board::emptySlot};

	private:
		uint32_t numTurns;
		Board::player_t numPlayers;
		Board::player_t currentPlayer;
		Board::player_t winner;
		Board board;
	};

	enum class ConnectDisks::TurnResult : uint8_t
	{
		success, wrongPlayer, badColumn
	};
}