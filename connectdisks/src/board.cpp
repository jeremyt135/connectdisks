#include "connectdisks/board.hpp"

#include <exception>

using namespace connectdisks;

Board::Board(Board::board_size_t numColumns, Board::board_size_t numRows) : numColumns{numColumns}, numRows{numRows}
{
	if (numColumns < minColumns || numRows < minRows || numColumns - numRows < 1)
	{
		throw std::invalid_argument("Board::Board: invalid board dimensions");
	}

	columns.resize(numColumns);
	for (auto& column : columns)
	{
		column.resize(numRows);
	}
	rowIndices.resize(numColumns);
}

Board::Board(Board && board) noexcept :
	numColumns{board.numColumns},
	numRows{board.numRows},
	columns{std::move(board.columns)},
	rowIndices{std::move(board.rowIndices)}
{
	board.numColumns = 0;
	board.numRows = 0;
}

Board::~Board()
{
}

Board& Board::operator=(Board && board) noexcept
{
	numColumns = board.numColumns;
	numRows = board.numRows;

	board.numColumns = 0;
	board.numRows = 0;

	columns = std::move(board.columns);
	rowIndices = std::move(board.rowIndices);

	return *this;
}

bool Board::isFull() const noexcept
{
	bool isBoardFull = true;

	for (board_size_t i = 0; i < numColumns; ++i)
	{
		if (!isColumnFullInternal(i))
		{
			isBoardFull = false;
			break;
		}
	}

	return isBoardFull;
}

bool Board::isColumnFull(Board::board_size_t column) const
{
	if (!isColumnInRange(column))
	{
		throw std::out_of_range("Board::isColumnFull: index out of range");
	}
	return isColumnFullInternal(column);
}

bool Board::dropPieceInColumn(Board::board_size_t column, Board::player_t playerNum)
{
	if (!isColumnInRange(column))
	{
		throw std::out_of_range("Board::dropPieceInColumn: index out of range");
	}

	if (isColumnFullInternal(column))
	{
		return false;
	}

	columns[column][rowIndices[column]++] = playerNum;
	return true;
}

Board::player_t Board::getPieceOwnerAt(Board::board_size_t column, Board::board_size_t row) const
{
	if (!isColumnInRange(column) || !isRowInRange(row))
	{
		throw std::out_of_range("Board::getPieceOwnerAt: index out of range");
	}

	return columns[column][row];
}

Board::column_view_t Board::getColumn(Board::board_size_t column) const
{
	if (!isColumnInRange(column))
	{
		throw std::out_of_range("Board::getColumn: index out of range");
	}
	return ColumnView{columns[column]};
}

Board::column_value_t  Board::getRow(Board::board_size_t row) const
{
	if (!isRowInRange(row))
	{
		throw std::out_of_range("Board::getRow: index out of range");
	}

	std::vector<player_t> boardRow;
	for (const auto& column : columns)
	{
		boardRow.push_back(column[row]);
	}
	return Column{std::move(boardRow)};
}

bool Board::isColumnFullInternal(Board::board_size_t column) const noexcept
{
	return rowIndices[column] == numRows;
}

inline bool Board::isColumnInRange(board_size_t column) const noexcept
{
	return column >= 0 && column < numColumns;
}

inline bool Board::isRowInRange(board_size_t row) const noexcept
{
	return row >= 0 && row < numRows;
}

Board::ColumnView::ColumnView(const Board::column_t& column) : column{column}
{
}

Board::column_t::const_iterator Board::ColumnView::begin() const noexcept
{
	return column.begin();
}

Board::column_t::const_iterator Board::ColumnView::end() const noexcept
{
	return column.end();
}

Board::Column::Column(Board::column_t && column) : column{column}
{
}


Board::column_t::const_iterator Board::Column::begin() const noexcept
{
	return column.begin();
}

Board::column_t::const_iterator Board::Column::end() const noexcept
{
	return column.end();
}