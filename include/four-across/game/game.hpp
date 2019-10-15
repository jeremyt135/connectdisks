#pragma once

#include "four-across/game/board.hpp"

#include <utility>

namespace game
{
	class FourAcross
	{
	public:
		FourAcross(
			uint8_t numPlayers = minNumPlayers,
			uint8_t firstPlayer = defaultFirstPlayer,
			uint8_t numColumns = Board::minColumns, 
			uint8_t numRows = Board::minRows);
		FourAcross(FourAcross&& connect) noexcept;
		~FourAcross();

		FourAcross& operator=(FourAcross&& connect) noexcept;

		enum class TurnResult : uint8_t;
		TurnResult takeTurn(uint8_t player, uint8_t column);

		inline bool hasWinner() const noexcept; 
		inline bool boardFull() const noexcept;

		inline uint8_t getWinner() const noexcept;

		// Return the id of the player who is taking their turn now
		inline uint8_t getCurrentPlayer() const noexcept;

		inline uint8_t getNumPlayers() const noexcept;
		inline uint32_t getNumTurns() const noexcept;

		inline uint8_t getNumColumns() const noexcept;
		inline uint8_t getNumRows() const noexcept;

		operator std::string() const;

		static constexpr uint8_t noWinner{Board::emptySlot};
		static constexpr uint8_t defaultFirstPlayer{1};
		static constexpr uint8_t minNumPlayers{2};
	private:
		uint8_t checkForWinner() const;
		uint8_t getNextPlayer() const noexcept;

		uint32_t numTurns;
		uint8_t numPlayers;
		uint8_t currentPlayer;
		std::pair<uint8_t, uint8_t> lastMove;
		uint8_t winner;
		Board board;
	};

	enum class FourAcross::TurnResult : uint8_t
	{
		error, success, wrongPlayer, badColumn, columnFull, gameFinished
	};


	inline bool FourAcross::hasWinner() const noexcept
	{
		return winner != noWinner;
	}

	inline bool FourAcross::boardFull() const noexcept
	{
		return board.isFull();
	}

	inline uint8_t FourAcross::getWinner() const noexcept
	{
		return winner;
	}

	inline uint8_t FourAcross::getCurrentPlayer() const noexcept
	{
		return currentPlayer;
	}

	inline uint8_t FourAcross::getNumPlayers() const noexcept
	{
		return numPlayers;
	}

	inline uint32_t FourAcross::getNumTurns() const noexcept
	{
		return numTurns;
	}

	inline uint8_t game::FourAcross::getNumColumns() const noexcept
	{
		return board.getNumColumns();
	}

	inline uint8_t game::FourAcross::getNumRows() const noexcept
	{
		return board.getNumRows();
	}
}

inline std::ostream& operator<<(std::ostream& out, const game::FourAcross& FourAcross)
{
	out << std::string{FourAcross};
	return out;
}
