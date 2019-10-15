#include "four-across/game/board.hpp"

#include <exception>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace game
{
	Board::Board(uint8_t numColumns, uint8_t numRows) : numColumns{numColumns}, numRows{numRows}
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

		for (uint8_t i = 0; i < numColumns; ++i)
		{
			if (!isColumnFullInternal(i))
			{
				isBoardFull = false;
				break;
			}
		}

		return isBoardFull;
	}

	bool Board::isColumnFull(uint8_t column) const
	{
		if (!isColumnInRange(column))
		{
			throw std::out_of_range("Board::isColumnFull: index out of range");
		}
		return isColumnFullInternal(column);
	}

	bool Board::dropPieceInColumn(uint8_t column, uint8_t playerNum)
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

	uint8_t Board::getDiskOwnerAt(uint8_t column, uint8_t row) const
	{
		if (!isColumnInRange(column) || !isRowInRange(row))
		{
			throw std::out_of_range("Board::getPieceOwnerAt: index out of range");
		}

		return columns[column][row];
	}

	Board::column_view_t Board::getColumn(uint8_t column) const
	{
		if (!isColumnInRange(column))
		{
			throw std::out_of_range("Board::getColumn: index out of range");
		}
		return ColumnView{columns[column]};
	}

	uint8_t game::Board::getColumnHeight(uint8_t column) const
	{
		if (!isColumnInRange(column))
		{
			throw std::out_of_range("Board::getColumn: index out of range");
		}
		return rowIndices[column];
	}

	Board::column_value_t  Board::getRow(uint8_t row) const
	{
		if (!isRowInRange(row))
		{
			throw std::out_of_range("Board::getRow: index out of range");
		}

		std::vector<uint8_t> boardRow;
		for (const auto& column : columns)
		{
			boardRow.push_back(column[row]);
		}
		return Column{std::move(boardRow)};
	}

	game::Board::operator std::string() const
	{
		std::ostringstream strStream;
		{
			// print top row first
			auto rowView = getRow(numRows - 1);
			strStream << " " << static_cast<int>(rowView[0]); // print first column of row
			for (int col = 1; col < static_cast<int>(numColumns); ++col)
			{
				strStream << " | " << static_cast<int>(rowView[col]);
			}
			strStream << "\n";
		}
		for (int row = static_cast<int>(numRows) - 2; row >= 0; --row)
		{
			// pre-print a row separator
			strStream << std::string(3 * numColumns + (numColumns - 1), '-') << "\n";

			// print the row
			auto rowView = getRow(row);
			strStream << " " << static_cast<int>(rowView[0]);
			for (int col = 1; col < static_cast<int>(numColumns); ++col)
			{
				strStream << " | " << static_cast<int>(rowView[col]);
			}
			strStream << "\n";
		}
		return strStream.str();
	}

	bool Board::isColumnFullInternal(uint8_t column) const noexcept
	{
		return rowIndices[column] == numRows;
	}

	inline bool Board::isColumnInRange(uint8_t column) const noexcept
	{
		return column >= 0 && column < numColumns;
	}

	inline bool Board::isRowInRange(uint8_t row) const noexcept
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

	Board::column_t::value_type game::Board::ColumnView::operator[](uint8_t index) const
	{
		return column[index];
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

	Board::column_t::value_type game::Board::Column::operator[](uint8_t index) const
	{
		return column[index];
	}

}

