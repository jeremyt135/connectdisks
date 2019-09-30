#include "connectdisks/connectdisks.hpp"

using namespace connectdisks;

ConnectDisks::ConnectDisks(Board::player_t numPlayers,
	Board::board_size_t numColumns, Board::board_size_t numRows) :
	board{numColumns, numRows},
	numPlayers{numPlayers},
	numTurns{0}
{
}

ConnectDisks::ConnectDisks(ConnectDisks && connect) noexcept:
	board{std::move(connect.board)},
	numTurns{connect.numTurns},
	numPlayers{connect.numPlayers}
{
	connect.numTurns = 0;
	connect.numPlayers = 0;
}

ConnectDisks::~ConnectDisks()
{
}

ConnectDisks::TurnResult ConnectDisks::takeTurn(Board::player_t player, Board::board_size_t column)
{
	return TurnResult();
}

inline bool ConnectDisks::hasWinner() const noexcept
{
	return false;
}

inline Board::player_t ConnectDisks::getWinner() const noexcept
{
	return Board::player_t();
}

inline Board::player_t ConnectDisks::getCurrentPlayer() const noexcept
{
	return Board::player_t();
}

inline Board::player_t ConnectDisks::getNumPlayers() const noexcept
{
	return Board::player_t();
}

inline uint32_t ConnectDisks::getNumTurns() const noexcept
{
	return uint32_t();
}

ConnectDisks & ConnectDisks::operator=(ConnectDisks && connect) noexcept
{
	board = std::move(connect.board);
	numTurns = connect.numTurns;
	numPlayers = connect.numPlayers;
	connect.numTurns = 0;
	connect.numPlayers = 0;
	return *this;
}
