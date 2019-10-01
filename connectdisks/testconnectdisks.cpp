#include "connectdisks/board.hpp"
#include "connectdisks/connectdisks.hpp"

#include <cassert>
#include <iostream>

int main(int argc, char* argv[])
{
	using namespace connectdisks;

	{
		// test default ConnectDisks
		ConnectDisks game{};

		// make first player win down a column
		Board::player_size_t firstPlayer = game.getCurrentPlayer();
		ConnectDisks::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // second player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);
		
		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == firstPlayer);
		

	}

	{
		// test default ConnectDisks
		ConnectDisks game{};

		// make first player win across a row
		Board::player_size_t firstPlayer = game.getCurrentPlayer();
		ConnectDisks::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 1); // second player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), game.getNumColumns() - 1);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == firstPlayer);
	}

	{
		// test default ConnectDisks
		ConnectDisks game{};

		// make first player win diagonally from bottom left
		Board::player_size_t firstPlayer = game.getCurrentPlayer();
		ConnectDisks::TurnResult result = {game.takeTurn(game.getCurrentPlayer(), 0)}; // first player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // second player's first move
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 1); // 1st starts stacking on top of 2nd player
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // 2nd player starts 3rd column (need 3 high)
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2);
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 2nd player starts 4th column (need 4 high)
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 2); // 1st player puts piece at top of 3rd column
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 4th column now at 2 high (2nd player's turn)
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 4th column at 3 high
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 0); // 2nd player dumps into 1st column
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == ConnectDisks::noWinner);

		result = game.takeTurn(game.getCurrentPlayer(), 3); // 1st should now win
		assert(result == ConnectDisks::TurnResult::success);
		assert(game.getWinner() == firstPlayer);
	}
}

