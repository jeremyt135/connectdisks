#include "four-across/game/game.hpp"

#include "logging.hpp"

#include <exception>
#include <iostream>
#include <sstream>

namespace game
{
	FourAcross::FourAcross(
		uint8_t numPlayers,
		uint8_t firstPlayer,
		uint8_t numColumns,
		uint8_t numRows) :
		numTurns{0},
		numPlayers{numPlayers >= minNumPlayers ? numPlayers : minNumPlayers},
		currentPlayer{firstPlayer > defaultFirstPlayer && firstPlayer <= numPlayers ? firstPlayer : defaultFirstPlayer},
		lastMove{0, 0},
		winner{noWinner},
		board{numColumns, numRows}
	{
	}

	FourAcross::FourAcross(FourAcross && connect) noexcept :
		numTurns{connect.numTurns},
		numPlayers{connect.numPlayers},
		currentPlayer{connect.currentPlayer},
		lastMove{std::move(connect.lastMove)},
		winner{connect.winner},
		board{std::move(connect.board)}
	{
		connect.numTurns = 0;
		connect.numPlayers = 0;
		connect.currentPlayer = 0;
		connect.lastMove.first = 0;
		connect.lastMove.second = 0;
		connect.winner = noWinner;
	}

	FourAcross::~FourAcross()
	{
	}

	FourAcross::TurnResult FourAcross::takeTurn(uint8_t player, uint8_t column)
	{
		if (winner != noWinner)
		{
			return TurnResult::gameFinished;
		}

		if (currentPlayer != player)
		{
			return TurnResult::wrongPlayer;
		}

		try
		{
			bool dropSuccessful = board.dropPieceInColumn(column, player);
			if (!dropSuccessful)
			{
				return TurnResult::columnFull;
			}
			else
			{
				// turn was valid
				currentPlayer = getNextPlayer();
				lastMove = std::make_pair<>(column, board.getColumnHeight(column) - 1);
				++numTurns;
				winner = checkForWinner();
			}
		}
		catch (std::out_of_range& error)
		{
			// invalid column number 
			printDebug("FourAcross::takeTurn: error taking turn:", error.what(), "\n");
			return TurnResult::badColumn;
		}
		catch (std::exception& error)
		{
			printDebug("FourAcross::takeTurn: fatal error taking turn", error.what(), "\n");
			throw;
		}

		return TurnResult::success;
	}

	FourAcross & FourAcross::operator=(FourAcross && connect) noexcept
	{
		board = std::move(connect.board);
		numTurns = connect.numTurns;
		numPlayers = connect.numPlayers;
		lastMove = std::move(connect.lastMove);
		winner = connect.winner;
		currentPlayer = connect.currentPlayer;
		connect.numTurns = 0;
		connect.numPlayers = 0;
		connect.lastMove.first = 0;
		connect.lastMove.second = 0;
		connect.winner = noWinner;
		connect.currentPlayer = 0;
		return *this;
	}

	uint8_t FourAcross::FourAcross::checkForWinner() const
	{
		const uint8_t lastColumn{lastMove.first};
		const uint8_t lastRow{lastMove.second};

		// assume the chain starts with the last move, and only check if
		// the last player won
		size_t count{1};
		const uint8_t lastPlayer{board.getDiskOwnerAt(lastColumn, lastRow)};
		const Board::column_view_t column{board.getColumn(lastColumn)};

		// check up the column (0 would be bottom, numRows - 1 would be top), starting above
		// the last move
		for (uint8_t row = lastRow + 1; row < board.getNumRows(); ++row)
		{
			uint8_t current = column[row];
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		// check down the column, starting below the last move
		for (uint8_t row = lastRow - 1; row != static_cast<uint8_t>(-1); --row)
		{
			uint8_t current = column[row];
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		// check across the row
		count = 1;
		const Board::column_value_t row{board.getRow(lastRow)};

		// scan row left to right, starting one to the right of last move
		for (uint8_t col = lastColumn + 1; col < board.getNumColumns(); ++col)
		{
			uint8_t current = row[col];
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		// check right to left, starting one to the left of last move
		for (uint8_t col = lastColumn - 1; col != static_cast<uint8_t>(-1); --col)
		{
			uint8_t current = row[col];
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		count = 1;

		// check up-right diagonal
		for (uint8_t row = lastRow + 1, col = lastColumn + 1;
			row < board.getNumRows() && col < board.getNumColumns();
			++row, ++col)
		{
			uint8_t current = board.getDiskOwnerAt(col, row);
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		// check down-left diagonal
		for (uint8_t row = lastRow - 1, col = lastColumn - 1;
			row != static_cast<uint8_t>(-1) && col != static_cast<uint8_t>(-1);
			--row, --col)
		{
			uint8_t current = board.getDiskOwnerAt(col, row);
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		count = 1;

		// check up-left diagonal
		for (uint8_t row = lastRow + 1, col = lastColumn - 1;
			row < board.getNumRows() && col != static_cast<uint8_t>(-1);
			++row, --col)
		{
			uint8_t current = board.getDiskOwnerAt(col, row);
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		// check down-right diagonal
		for (uint8_t row = lastRow - 1, col = lastColumn + 1;
			row != static_cast<uint8_t>(-1) && col < board.getNumColumns();
			--row, ++col)
		{
			uint8_t current = board.getDiskOwnerAt(col, row);
			if (current != lastPlayer)
			{
				break;
			}
			else
			{
				++count;
				if (count == 4)
				{
					return lastPlayer;
				}
			}
		}

		return noWinner;
	}

	uint8_t FourAcross::FourAcross::getNextPlayer() const noexcept
	{
		uint8_t nextPlayer = currentPlayer + 1; // may overflow to 0
		if (nextPlayer > numPlayers || nextPlayer == 0)
		{
			nextPlayer = defaultFirstPlayer; // wrap around
		}
		return nextPlayer;
	}

	FourAcross::FourAcross::operator std::string() const
	{
		std::ostringstream strStream;
		strStream << board << "\n" <<
			"Current player: " << static_cast<int>(currentPlayer) <<
			" Next player: " << static_cast<int>(getNextPlayer()) <<
			"\nNum turns: " << static_cast<int>(numTurns) << "\n";
		if (hasWinner())
		{
			strStream << "Winner: " << static_cast<int>(winner) << "\n";
		}
		return strStream.str();
	}
}