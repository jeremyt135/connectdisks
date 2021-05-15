#include "four-across/game/board.hpp"
#include "four-across/game/game.hpp"

#include <cassert>
#include <iostream>

int main(int argc, char* argv[])
{
	using namespace game;
	{
		// test default FourAcross
		FourAcross game{};

		// make first player win down a column
		uint8_t firstPlayer = game.getCurrentPlayer();
		FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // second player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == firstPlayer);

		std::cout << game << "\n";
	}

	{
		// test default FourAcross
		FourAcross game{};

		// make first player win across a row
		uint8_t firstPlayer = game.getCurrentPlayer();
		FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 1); // second player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == firstPlayer);

		std::cout << game << "\n";
	}

	{
		// test default FourAcross
		FourAcross game{};

		// make first player win diagonally from bottom left
		uint8_t firstPlayer = game.getCurrentPlayer();
		FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // second player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // 1st starts stacking on top of 2nd player
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // 2nd player starts 3rd column (need 3 high)
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 2nd player starts 4th column (need 4 high)
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // 1st player puts piece at top of 3rd column
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 4th column now at 2 high (2nd player's turn)
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 4th column at 3 high
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // 2nd player dumps into 1st column
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 1st should now win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == firstPlayer);

		std::cout << game << "\n";
	}

	{
		// default is 4 rows, so use 6 instead to be sure the "win from the top" condition is checked
		const uint8_t columns{7};
		const uint8_t rows{6};
		FourAcross game{2, 1, columns, rows};

		// make first player win starting at the top of a column (need 2 "extra" at the bottom of the column)
		uint8_t firstPlayer = game.getCurrentPlayer();
		FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // second player's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // first player starting their winning chain
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // second player placing pieces wherever
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // 2 more to win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // 1 more to win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // 1st should win now
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == firstPlayer);

		std::cout << game << "\n";
	}

	{
		// make 2nd player start the game
		FourAcross game{2, 2};

		// make player 1 win starting from the right end of a row (need 3 "extra" pieces)
		uint8_t winner = 1;
		FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // player 2's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // player 1's first move
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // need 2 more to win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // need 1 more to win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 4); // first player should win
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == winner);

		std::cout << game << "\n";
	}

	{
		// make 2nd player start the game
		FourAcross game{2, 2};

		// make player 1 win diagonally from the topright corner on a default (5x4) board
		uint8_t winner = 1;

		// drop pieces in last column (player 1's piece will be at top)
		for (auto i = 0; i < 4; ++i)
		{
			FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 1)};
			assert(result == FourAcross::TurnResult::success);
			assert(game.getWinner() == FourAcross::noWinner);
		}

		// player 2 places piece at bottom of 2nd from last column
		FourAcross::TurnResult result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// player 1 places piece on top of previous
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 2nd player drops piece in 3rd from last column
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 3);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 1st player drops piece in 2nd from last column (2 more to win)
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 2nd player drops wherever that isn't blocking
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 1st player places piece in 3rd from right column (1 more to win)
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 3);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);


		// 2nd player drops wherever that isn't blocking
		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 1st player places winning piece
		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 4);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == winner);

		std::cout << game << "\n";
	}

	{
		// make 2nd player start the game
		FourAcross game{2, 2};

		// make player 1 win diagonally from topleft downwards to right
		uint8_t winner = 1;

		// get player 1's piece to top of first column
		for (auto i = 0; i < 4; ++i)
		{
			FourAcross::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)};
			assert(result == FourAcross::TurnResult::success);
			assert(game.getWinner() == FourAcross::noWinner);
		}


		// player 2's turn
		FourAcross::TurnResult result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 4);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 4);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);
		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == FourAcross::noWinner);

		// 1st player places winning piece
		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == FourAcross::TurnResult::success);
		assert(game.getWinner() == winner);

		std::cout << game << "\n";
	}

	std::cout << "tests passed\n";
}