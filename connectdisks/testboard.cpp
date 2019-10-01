#include "connectdisks/board.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
	using namespace connectdisks;
	const Board::board_size_t columns{7};
	const Board::board_size_t rows{6};
	Board board{columns, rows};
	std::cout << "created board(numColumns=" << static_cast<int>(board.getNumColumns()) << ", numRows=" << static_cast<int>(board.getNumRows()) << ")\n";
	Board::player_size_t player = 1;
	for (int i = 0; i < columns; ++i)
	{
		board.dropPieceInColumn(i, player);
		board.dropPieceInColumn(columns - 1, player + 1);
	}

	{
		Board::player_size_t playerAt = board.getDiskOwnerAt(0, 0);
		std::cout << "player at 0,0: " << static_cast<int>(playerAt) << "\n";
	}
	
	{
		std::cout << "first column:\n";
		Board::column_view_t column = board.getColumn(0);
		for (auto& playerId : column)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}
	
	{
		std::cout << "second column:\n";
		Board::column_view_t column = board.getColumn(1);
		for (auto& playerId : column)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}

	{
		std::cout << "first row:\n";
		Board::column_value_t row = board.getRow(0);
		for (auto& playerId : row)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}

	for (int i = 0; i < columns; ++i)
	{
		for (int j = 0; j < rows; ++j)
		{
			board.dropPieceInColumn(i, player);
		}
	}

	{
		std::cout << "last row:\n";
		Board::column_value_t row = board.getRow(rows - 1);
		for (auto& playerId : row)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}

	std::cout << "is board full?: " << board.isFull() << "\n";

	try
	{
		Board board1{0, 0};
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		Board board1{columns, 0};
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		Board board1{0, rows};
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		Board board1{columns - 2u * (columns - rows), rows};
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.dropPieceInColumn(columns + 1, 0);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}
	try
	{
		board.isColumnFull(columns + 1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getDiskOwnerAt(columns + 1, 0);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getDiskOwnerAt(0, rows + 1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getDiskOwnerAt(-1, rows - 1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getDiskOwnerAt(columns - 1, -1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getColumn(-1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}
	try
	{
		board.getColumn(columns + 1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	try
	{
		board.getRow(-1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}
	try
	{
		board.getRow(rows + 1);
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	Board board1{std::move(board)};
	std::cout << "moved board->board1, board1(numColumns=" 
		<< static_cast<int>(board1.getNumColumns()) << ", numRows=" << static_cast<int>(board1.getNumRows()) << ")\n";

	{
		Board::column_view_t column = board1.getColumn(0);
		for (auto& playerId : column)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}
	
	try
	{
		Board::column_view_t column = board.getColumn(0);
		for (auto& playerId : column)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}

	Board board2{};
	{
		Board board3{std::move(board2)};
	}
	try
	{
		Board::column_view_t column = board2.getColumn(0);
		for (auto& playerId : column)
		{
			std::cout << static_cast<int>(playerId) << "\n";
		}
	}
	catch (std::exception& e)
	{
		std::cout << "caught exception: " << e.what() << "\n";
	}
}