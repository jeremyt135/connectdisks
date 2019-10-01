#pragma once

#include "connectdisks/board.hpp"

#include <utility>

namespace connectdisks
{
	class ConnectDisks
	{
	public:
		ConnectDisks(
			Board::player_size_t numPlayers = {2},
			Board::player_size_t firstPlayer = {firstPlayerId},
			Board::board_size_t numColumns = Board::minColumns, 
			Board::board_size_t numRows = Board::minRows);
		ConnectDisks(ConnectDisks&& connect) noexcept;
		~ConnectDisks();

		enum class TurnResult : uint8_t;
		TurnResult takeTurn(Board::player_size_t player, Board::board_size_t column);

		inline bool hasWinner() const noexcept; 

		inline Board::player_size_t getWinner() const noexcept;

		// Return the id of the player who is taking their turn now
		inline Board::player_size_t getCurrentPlayer() const noexcept;

		inline Board::player_size_t getNumPlayers() const noexcept;
		inline uint32_t getNumTurns() const noexcept;

		ConnectDisks& operator=(ConnectDisks&& connect) noexcept;

		static constexpr Board::player_size_t noWinner{Board::emptySlot};
		static constexpr Board::player_size_t firstPlayerId{1};
	private:
		Board::player_size_t checkForWinner() const;
		Board::player_size_t getNextPlayer() const noexcept;

		uint32_t numTurns;
		Board::player_size_t numPlayers;
		Board::player_size_t currentPlayer;
		std::pair<Board::board_size_t, Board::board_size_t> lastMove;
		Board::player_size_t winner;
		Board board;
	};

	enum class ConnectDisks::TurnResult : uint8_t
	{
		success, wrongPlayer, badColumn, columnFull, gameFinished
	};


	inline bool ConnectDisks::hasWinner() const noexcept
	{
		return winner != noWinner;
	}

	inline Board::player_size_t ConnectDisks::getWinner() const noexcept
	{
		return winner;
	}

	inline Board::player_size_t ConnectDisks::getCurrentPlayer() const noexcept
	{
		return currentPlayer;
	}

	inline Board::player_size_t ConnectDisks::getNumPlayers() const noexcept
	{
		return numPlayers;
	}

	inline uint32_t ConnectDisks::getNumTurns() const noexcept
	{
		return numTurns;
	}

}