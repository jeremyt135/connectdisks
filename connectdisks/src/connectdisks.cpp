#include "connectdisks/connectdisks.hpp"

#include <iostream>
#include <exception>

using namespace connectdisks;

ConnectDisks::ConnectDisks(
	Board::player_size_t numPlayers,
	Board::player_size_t firstPlayer,
	Board::board_size_t numColumns,
	Board::board_size_t numRows) :
	numTurns{0},
	numPlayers{numPlayers},
	currentPlayer{firstPlayer > firstPlayerId && firstPlayer <= numPlayers ? firstPlayer : firstPlayerId},
	lastMove{0, 0},
	winner{noWinner},
	board{numColumns, numRows}
{
}

ConnectDisks::ConnectDisks(ConnectDisks && connect) noexcept :
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

ConnectDisks::~ConnectDisks()
{
}

ConnectDisks::TurnResult ConnectDisks::takeTurn(Board::player_size_t player, Board::board_size_t column)
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
	catch (std::out_of_range& e)
	{
		// invalid column number 
	#if defined DEBUG || defined _DEBUG
		std::cerr << "ConnectDisks::takeTurn: " << e.what() << "\n";
	#endif
		return TurnResult::badColumn;
	}
	catch (std::exception& e)
	{
	#if defined DEBUG || defined _DEBUG
		std::cerr << "ConnectDisks::takeTurn: " << e.what() << "\n";
	#endif
		throw;
	}

	return TurnResult::success;
}

ConnectDisks & ConnectDisks::operator=(ConnectDisks && connect) noexcept
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

Board::player_size_t connectdisks::ConnectDisks::checkForWinner() const
{
	const Board::board_size_t lastColumn{lastMove.first};
	const Board::board_size_t lastRow{lastMove.second};

	// assume the chain starts with the last move, and only check if
	// the last player won
	size_t count{1};
	const Board::player_size_t lastPlayer{board.getDiskOwnerAt(lastColumn, lastRow)};
	const Board::column_view_t column{board.getColumn(lastColumn)};

	// check up the column (0 would be bottom, numRows - 1 would be top), starting above
	// the last move
	for (int32_t row = lastRow + 1; row < board.getNumRows(); ++row)
	{
		Board::player_size_t current = column[row];
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
	for (int32_t row = lastRow - 1; row >= 0; --row)
	{
		Board::player_size_t current = column[row];
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
	for (int32_t col = lastColumn + 1; col < board.getNumColumns(); ++col)
	{
		Board::player_size_t current = row[col];
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
	for (int32_t col = lastColumn - 1; col >= 0; --col)
	{
		Board::player_size_t current = row[col];
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

	// check up diagonal
	for (int32_t row = lastRow + 1, col = lastColumn + 1;
		row < board.getNumRows() && col < board.getNumColumns();
		++row, ++col)
	{
		Board::player_size_t current = board.getDiskOwnerAt(col, row);
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


	// check down diagonal
	for (int32_t row = lastRow - 1, col = lastColumn - 1;
		row >= 0 && col >= 0;
		--row, --col)
	{
		Board::player_size_t current = board.getDiskOwnerAt(col, row);
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

Board::player_size_t connectdisks::ConnectDisks::getNextPlayer() const noexcept
{
	Board::player_size_t nextPlayer = currentPlayer + 1; // may overflow to 0
	if (nextPlayer > numPlayers || nextPlayer == 0)
	{
		nextPlayer = firstPlayerId; // wrap around
	}
	return nextPlayer;
}
