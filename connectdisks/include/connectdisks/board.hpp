#pragma once

#include <limits>
#include <vector>

namespace connectdisks
{
	// Game board for Connect Four
	class Board
	{
		class ColumnView;
		class Column;
	public:
		using player_size_t = uint8_t;
		using board_size_t = uint8_t;

		using column_view_t = ColumnView;
		using column_value_t = Column;

		// Constructs a Board; throws std::runtime_exception if numColumns < 7, numRows  < 6, 
		// or numColumns - numRows < 1
		Board(board_size_t numColumns = minColumns, board_size_t numRows = minRows);

		// Move constructs from existing board: leaves argument in a destructable but unusable state.
		// Any modifiers or queries that need to access the underlying grid data will throw out_of_range exceptions.
		Board(Board&& board) noexcept;

		~Board();
		
		// Move assigns from existing board: see move constructor for usability after move.
		Board& operator=(Board&& board) noexcept;

		// Attempts to drop a player piece into given column, returning false if column is full.
		bool dropPieceInColumn(board_size_t column, player_size_t playerNum);

		// Returns true if entire board is full.
		bool isFull() const noexcept;

		// Returns true if column is full.
		bool isColumnFull(board_size_t column) const;

		// Gets the owner of the piece at given column, row. Returns Board::emptySlot if slot is empty.
		player_size_t getDiskOwnerAt(board_size_t column, board_size_t row) const;

		// Gets a view of an entire column of the board.
		column_view_t getColumn(board_size_t column) const;

		// Get the height of a column (number of pieces in it)
		board_size_t getColumnHeight(board_size_t column) const;

		// Gets a view of an entire row of the board.
		column_value_t getRow(board_size_t row) const;

		inline board_size_t getNumColumns() const noexcept;
		inline board_size_t getNumRows() const noexcept;

		// Nonplayer value of grid slots.
		static constexpr player_size_t emptySlot{0};

		// Minimum allowed value for column and row.
		static constexpr board_size_t minColumns{5};
		static constexpr board_size_t minRows{4};
	private:
		using column_t = std::vector<player_size_t>;
		using grid_t = std::vector<column_t>;

		board_size_t numColumns;
		board_size_t numRows;

		grid_t columns;
		std::vector<board_size_t> rowIndices;

		inline bool isColumnFullInternal(board_size_t column) const noexcept;

		inline bool isColumnInRange(board_size_t column) const noexcept;
		inline bool isRowInRange(board_size_t row) const noexcept;
	};

	Board::board_size_t Board::getNumColumns() const noexcept
	{
		return numColumns;
	}

	Board::board_size_t Board::getNumRows() const noexcept
	{
		return numRows;
	}

	class Board::ColumnView
	{
		friend class Board;
	public:
		column_t::const_iterator begin() const noexcept;
		column_t::const_iterator end() const noexcept;
		column_t::value_type operator[](board_size_t index) const;
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
		column_t::value_type operator[](board_size_t index) const;
	private:
		explicit Column(column_t&& column);
		column_t column;
	};
}