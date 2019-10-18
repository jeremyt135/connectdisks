#pragma once

#include <limits>
#include <string>
#include <vector>

namespace game
{
	// Game board for Four Across games.
	// The board is indexed starting at 0,0 for the bottom left of the board
	// up to numColumns - 1, numRows - 1 for the top right.
	class Board
	{
		class ColumnView; // allows read operations on a reference to a column
		class Column; // allows read operations on a copy of a column
	public:

		using column_view_t = ColumnView;
		using column_value_t = Column;

		// Constructs a Board; throws std::runtime_exception if numColumns < 7, numRows  < 6, 
		// or numColumns - numRows < 1
		Board(uint8_t numColumns = minColumns, uint8_t numRows = minRows);

		// Move constructs from existing board: leaves argument in a destructable but unusable state.
		// Any modifiers or queries that need to access the underlying grid data will throw out_of_range exceptions.
		Board(Board&& board) noexcept;

		~Board();

		// Move assigns from existing board: see move constructor for usability after move.
		Board& operator=(Board&& board) noexcept;

		// Attempts to drop a player piece into given column, returning false if column is full.
		bool dropPieceInColumn(uint8_t column, uint8_t playerNum);

		// Returns true if entire board is full.
		bool isFull() const noexcept;

		// Returns true if column is full.
		bool isColumnFull(uint8_t column) const;

		// Gets the owner of the piece at given column, row. Returns Board::emptySlot if slot is empty.
		uint8_t getDiskOwnerAt(uint8_t column, uint8_t row) const;

		// Gets a view of an entire column of the board.
		column_view_t getColumn(uint8_t column) const;

		// Get the height of a column (number of pieces in it)
		uint8_t getColumnHeight(uint8_t column) const;

		// Gets a view of an entire row of the board.
		column_value_t getRow(uint8_t row) const;

		uint8_t getNumColumns() const noexcept;
		uint8_t getNumRows() const noexcept;

		operator std::string() const;

		// Nonplayer value of grid slots.
		static constexpr uint8_t emptySlot{0};

		// Minimum allowed value for column and row.
		static constexpr uint8_t minColumns{5};
		static constexpr uint8_t minRows{4};
	private:
		using column_t = std::vector<uint8_t>;
		using grid_t = std::vector<column_t>;

		uint8_t numColumns;
		uint8_t numRows;

		grid_t columns;
		std::vector<uint8_t> rowIndices;

		bool isColumnFullInternal(uint8_t column) const noexcept;

		bool isColumnInRange(uint8_t column) const noexcept;
		bool isRowInRange(uint8_t row) const noexcept;
	};

	inline uint8_t Board::getNumColumns() const noexcept
	{
		return numColumns;
	}

	inline uint8_t Board::getNumRows() const noexcept
	{
		return numRows;
	}

	class Board::ColumnView
	{
		friend class Board;
	public:
		column_t::const_iterator begin() const noexcept;
		column_t::const_iterator end() const noexcept;
		column_t::value_type operator[](uint8_t index) const;
	private:
		explicit ColumnView(const column_t& column);
		const column_t& column;
	};

	class Board::Column
	{
		friend class Board;
	public:
		column_t::const_iterator begin() const noexcept;
		column_t::const_iterator end() const noexcept;
		column_t::value_type operator[](uint8_t index) const;
	private:
		explicit Column(column_t&& column);
		column_t column;
	};
}

inline std::ostream& operator<<(std::ostream& out, const game::Board& board)
{
	out << std::string{board};
	return out;
}
