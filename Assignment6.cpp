/*
	<@
	(KU/
	 "
human.cpp - by google gemini flash
modified slightly by lucas frias
 Program:       EECS 348 Assignment 6 - Sudoku solver (Assignment6.cpp, revised from fox.cpp/human.cpp)
 Description:  Solves Sudoku problems by depth-first search with backtracking and finds EVERY solution.
               Each solution is printed to the console and also saved to one shared file, solutions.txt.
               Assignment 5 revision: digits are tracked in bitmasks, the empty cell with the fewest
               legal digits is filled first, and contradictory givens are detected while loading.
               Assignment 6 revision: every puzzle's output now goes into that one shared solutions.txt
               instead of a separate solutionN.txt per puzzle.
 Input:         puzzle1.txt ... puzzle5.txt (9 rows of 9 cells, '_' = empty cell, 1-9 = given digit)
 Output:        Console report for every puzzle, plus solutions.txt. For each puzzle the file holds the line
               "Puzzle: x" (x = puzzle file name), then either all solution grids (same layout as the puzzle
               files, one blank line after each grid) followed by the single line "Solutions found: n", or just
               the single line "No solution found.". A blank line separates one puzzle's section from the next
 Collaborators:  Google Gemini Flash, Lucas Frias, Jacob Fox, Claude (Anthropic)
 Other sources:  Gemini Flash wrote the Assignment 4 base; Claude (Anthropic) wrote the Assignment 5 changes
               (prompted by Jacob Fox) and the Assignment 6 changes (prompted by Lucas Frias)
 Block tags:    [A5-Claude]  Assignment 5 code, written by Claude and reviewed by Jacob Fox
               [A6-Claude]  Assignment 6 code (the shared solutions.txt change), written by Claude
                            and reviewed by Lucas Frias
               (no tag)     Assignment 4 code kept as received (Gemini Flash / Lucas Frias), or a
                            personal comment Lucas Frias added while reviewing
 * Author:        Lucas Frias (by using Gemini Flash)
 * Created:       October 13th 2026
 * Revised by:    Jacob Fox, 2026-10-26 (Assignment 5: solution files, faster search, input checks)
 * Revised by:    Lucas Frias, 2026-11-11 (Assignment 6: single shared solutions.txt)
 * Build:         g++ Assignment6.cpp -std=c++11 -Wall -Wextra -o Assignment6.exe
 * Run:           ./Assignment6.exe (puzzle1.txt through puzzle5.txt must be in the same folder)
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

class SudokuSolver {
//sudoku solver, our main class to solve sudoku

private:
    //make a private class with a size, blcok_size
    static const int SIZE = 9;
    static const int BLOCK_SIZE = 3;
    // [A5-Claude] all nine digits as one bitmask: bit d stands for digit d, so bits 1..9 are on and bit 0 is
    // unused (0x3FE when SIZE is 9). sets of candidate digits are stored as masks like this one
    static const unsigned int ALL_DIGITS = (1u << (SIZE + 1)) - 2;
    //make a grid with our set size (always a square)
    int grid[SIZE][SIZE];
    // [A5-Claude] bookkeeping for the sudoku rules. bit d of rowUsed[r] is on when digit d is already in row r;
    // colUsed and boxUsed do the same for columns and 3x3 boxes. a digit can then be tested with one and
    // instead of scanning 27 cells. the "= {}" starts every mask at zero (no digits used yet)
    unsigned int rowUsed[SIZE] = {};
    unsigned int colUsed[SIZE] = {};
    unsigned int boxUsed[SIZE] = {};
    // [A5-Claude] turns true if two given digits break a sudoku rule, which means no solution can exist
    bool givensConflict = false;
    //make a filename
    std::string filename;
    // [A6-Claude] the shared solutions.txt stream. main() opens it once and every solver writes its own
    // section into it. answers are written the moment they are found, so no solution is kept in memory
    std::ostream& solutionFile;
    //make a counter for the solutions found
    //lucas edit, the solutionsFound should be zero
    //initialised when declared
    int solutionsFound = 0;

    // checks if placing 'num' at grid[row][col] is valid

    bool isValid(int row, int col, int num) const {
        // [A5-Claude] the old body scanned the row, column and box (27 cells) on every try. now it is a
        // single bit test: candidates() already knows which digits are legal here, so shift that mask right by
        // 'num' and look at the lowest bit (1 means digit 'num' is legal in this cell)
        return ((candidates(row, col) >> num) & 1u) != 0;
    }

    // [A5-Claude] tells which of the nine 3x3 boxes a cell sits in. boxes are numbered 0..8, left to right and
    // then top to bottom, so the centre box is number 4. it picks the right entry of boxUsed
    int boxOf(int row, int col) const {
        // row / 3 gives the band of boxes (0, 1 or 2); col / 3 gives the box inside that band
        return (row / BLOCK_SIZE) * BLOCK_SIZE + (col / BLOCK_SIZE);
    }

    // [A5-Claude] returns the digits that may still legally go into cell (row, col), one bit per digit.
    // this is the only place where the row, column and box rules of sudoku are checked
    unsigned int candidates(int row, int col) const {
        // or the three "already used" masks together: a 1 bit means that digit is blocked for this cell
        unsigned int blocked = rowUsed[row] | colUsed[col] | boxUsed[boxOf(row, col)];
        // keep only the digit bits (1..9) that are not blocked
        return ALL_DIGITS & ~blocked;
    }

    // [A5-Claude] counts the 1 bits in a mask, i.e. how many candidate digits a cell has left
    static int countBits(unsigned int mask) {
        int count = 0;          // running total of 1 bits seen so far
        while (mask != 0) {     // keep going until no 1 bits are left
            mask &= mask - 1;   // switch off the lowest 1 bit
            count++;            // and count it
        }
        return count;           // 0..9 for our masks
    }

    // [A5-Claude] puts a digit into a cell and records it in the row, column and box masks
    void place(int row, int col, int num) {
        const unsigned int bit = 1u << num;   // one-hot mask of this digit (digit 5 -> binary 100000)
        grid[row][col] = num;                 // write the digit into the grid
        rowUsed[row] |= bit;                  // the digit is now used in this row
        colUsed[col] |= bit;                  // ... in this column
        boxUsed[boxOf(row, col)] |= bit;      // ... and in this box
    }

    // [A5-Claude] exact reverse of place(): empties the cell and frees its digit in the three masks
    void unplace(int row, int col) {
        const unsigned int bit = 1u << grid[row][col];  // which digit is being taken back out
        rowUsed[row] &= ~bit;                           // free it in this row
        colUsed[col] &= ~bit;                           // ... in this column
        boxUsed[boxOf(row, col)] &= ~bit;               // ... and in this box
        grid[row][col] = 0;                             // the cell is empty again
    }

    // [A5-Claude] runs once for every completed grid the search reaches. keeping this out of solveRecursive
    // means the search code only searches, and all the output work for a solution lives in one place.
    // [A6-Claude] solutions no longer print to std::cout here: a puzzle with many solutions was spending
    // most of its time on console writes instead of search, since every grid went to the terminal as well
    // as the file. writing only to solutionFile keeps the per-solution cost to the file write
    void recordSolution() {
        solutionsFound++;            // one more solution
        printGrid(solutionFile);     // file copy, same layout as the puzzle files
        solutionFile << "\n";        // blank line so grids in the file stay apart
    }

    // recursive helper to find all solutions using depth-first search with backtracking
    void solveRecursive() {
	// gets the recursive solution
        // [A5-Claude] no row/col arguments any more. instead of walking the grid left to right, every call
        // looks for the empty cell with the fewest legal digits and branches on that one. a cell with one
        // option is a forced move and a cell with zero options is a dead end that is spotted immediately,
        // so far fewer branches are tried than before
        int bestRow = -1;            // row of the most constrained empty cell found so far (-1 = none found yet)
        int bestCol = -1;            // column of that same cell
        unsigned int bestMask = 0;   // that cell's legal digits, one bit per digit
        int bestCount = SIZE + 1;    // how many legal digits it has; starts above the maximum possible (9)

        // visit every cell once: cell numbers 0..80 map to row = cell / 9 and col = cell % 9
        for (int cell = 0; cell < SIZE * SIZE; cell++) {
            int row = cell / SIZE;                         // row of this cell
            int col = cell % SIZE;                         // column of this cell
            if (grid[row][col] != 0) {                     // already filled (a given or a placed digit)
                continue;                                  // nothing to decide here, go to the next cell
            }
            unsigned int mask = candidates(row, col);      // the digits that could still go into this cell
            int count = countBits(mask);                   // and how many of them there are
            // a cell with zero candidates proves this branch cannot work.
            if (count == 0) {
                bestRow = row;
                bestCol = col;
                bestMask = 0;
                bestCount = 0;
                break;
            }

            // otherwise, remember the cell with the fewest candidates.
            if (count < bestCount) {
                bestRow = row;
                bestCol = col;
                bestMask = mask;
                bestCount = count;
            }
        }
    

        // no empty cell was found, so the grid is full. every digit went in through the masks, so it is a
        // valid solution
        if (bestRow == -1) {
            recordSolution();       // count it, print it and save it
            return;                 // back out and keep looking for other solutions
        }

        // branch on the chosen cell, trying its legal digits in ascending order. if it has no legal digits,
        // bestMask is 0, no digit passes the test below and the loop does nothing, so a dead end simply ends
        // this branch with no special case
	//see canidates 1-9 on our current element
        for (int num = 1; num <= SIZE; num++) {
            if ((bestMask >> num) & 1u) {         // [A5-Claude] is digit 'num' one of this cell's legal digits?
                place(bestRow, bestCol, num);     // put the digit down and update the masks
                solveRecursive();                 // search the rest of the grid
                unplace(bestRow, bestCol);        // backtrack: take it out again and try the next digit
            }
        }
    }

public:
	//initialiser method, takes a sting and requires a filename
	//calculates the solutionsFound
    // [A6-Claude] now also receives the shared solutions.txt stream this puzzle writes its section into
    SudokuSolver(const std::string& file, std::ostream& out) : filename(file), solutionFile(out), solutionsFound(0) {
        for (int row = 0; row < SIZE; row++) {
            for (int col = 0; col < SIZE; col++) {
		//zero intiialise all data
                grid[row][col] = 0;
            }
        }
    }

    // reads the grid from the file
    bool loadPuzzle() {
	//make a file stream
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return false;
        }
	//change -> intialised unitiliased zero	
	//explicitly zero initialising this is a good idea
	//so i did it
        char cell = 0;
		
        for (int row = 0; row < SIZE; row++) {
            for (int col= 0; col < SIZE; col++) {
		//copies file data into cell char by char
                if (!(file >> cell)) {
			//means we can't pull any new chars which means we're short in characters
                    std::cerr << "Error: Invalid or incomplete puzzle format in " << filename << std::endl;
                    return false;
                }
		//here we check to see the data return type
		//in this case, we're seeing if it's empty
		//or some char 1-9 or a bad char
                if (cell == '_') { 
                    grid[row][col] = 0;
                } else if (cell >= '1' && cell <= '9') {
                    // [A5-Claude] a given digit. if its row, column or box already holds the same digit, two
                    // givens clash and no solution can exist. either way the digit is stored and added to the masks
                    int digit = cell - '0';              // the character '7' becomes the number 7
                    if (!isValid(row, col, digit)) {     // this digit is already used in the row/column/box
                        givensConflict = true;           // remember the clash (checked later in solve())
                    }
                    place(row, col, digit);              // store the digit and update the three masks
                } else {
                    std::cerr << "Error: Invalid character '" << cell << "' in " << filename << std::endl;
                    return false;
                }
            }
        }
        // [A5-Claude] all 81 cells are read. if another non-blank character is still in the file it is not a
        // 9x9 puzzle (a 10th column would silently shift every row), so refuse the file instead of ignoring it
        if (file >> cell) {
            std::cerr << "Error: Extra data after the 81st cell in " << filename << std::endl;
            return false;
        }
	//load puzzle returns a boolean, which is kinda weird but not 
	//too bad of a choice. it makes sense to do the init method
        return true;
    }

    // prints the grid
    // [A5-Claude] the output stream is now a parameter, so one function prints to the console and to the file
    void printGrid(std::ostream& out) const {
	//for all the rows, columns, display either
	//the empty _ char to represent a zero possibility
	//or displsy a number 1-9 to output what the value
	//ought to be when displaying
        for (int row= 0; row < SIZE; row++) {
            for (int col = 0; col < SIZE; col++) {
                if (grid[row][col] == 0) {
                    out << "_ ";
                } else {
                    out << grid[row][col] << " ";
                }
            }
            out << "\n";
        }
    }

    // [A6-Claude] writes the section for a puzzle that could not be loaded (missing file or bad format), so
    // every listed puzzle still appears in solutions.txt. an unreadable puzzle has no solution to report
    void recordLoadFailure() {
        solutionFile << "Puzzle: " << filename << "\n";
        solutionFile << "No solution found.\n\n";
    }

    // primary object method to run solving routine and output formatted results
    void solve() {
	//display the filename and the puzzle
        std::cout << "========================================\n";
        std::cout << "File: " << filename << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Initial Puzzle:\n";
        printGrid(std::cout);
        // [A6-Claude] start this puzzle's section in solutions.txt with its header line
        solutionFile << "Puzzle: " << filename << "\n";

	//here we print all solutions by just recursively
	//going and display stuff. solutionsFound is a public
	//var so we have space for it.
        solutionsFound = 0;
        // [A5-Claude] if two givens clash there is nothing to search; otherwise run the search
        if (givensConflict) {
            std::cout << "\nTwo of the given digits clash, so this puzzle has no solution.\n";
        } else {
            solveRecursive();
        }
	

	//display solutions found as a either a no solutions
	//or as a count, actual printing is handled later
        if (solutionsFound == 0) {
            std::cout << "\nNo solution found\n";
            solutionFile << "No solution found.\n";                          // [A5-Claude] file version: exactly this one line
        } else {
            std::cout << "\nSolutions found: " << solutionsFound << "\n";
            solutionFile << "Solutions found: " << solutionsFound << "\n";   // [A5-Claude] file version: count line after all grids
        }
        solutionFile << "\n";   // [A6-Claude] blank line between this puzzle's section and the next one
        std::cout << "========================================\n\n";
    }
};

int main() {
   ///get all the puzzles
    std::vector<std::string> puzzleFiles = {
        "puzzle1.txt",
        "puzzle2.txt",
        "puzzle3.txt",
        "puzzle4.txt",
        "puzzle5.txt"
    };
    // [A6-Claude] one shared output file for every puzzle, created (or emptied) once before the loop
    // (Assignment 5 opened/closed a separate solutionN.txt per puzzle instead)
    std::ofstream solutionsFile("solutions.txt");
    if (!solutionsFile.is_open()) {
        std::cerr << "Error: Could not create output file solutions.txt" << std::endl;
        return 1;
    }
    //use dynamic iteration if we wanna add more puzzles
    for (const auto& filename : puzzleFiles) {
	//make a sudoku instance
        SudokuSolver solver(filename, solutionsFile);   // [A6-Claude] every solver writes into solutions.txt
	//can we load it?
        if (solver.loadPuzzle()) {
            solver.solve();//then solve it, which prints everything
        } else {
            solver.recordLoadFailure();   // [A6-Claude] still give the puzzle its section in solutions.txt
        }
    }
    // [A5-Claude] close the file (this also flushes it), then tell the user whether the save worked
    solutionsFile.close();
    if (solutionsFile.fail()) {
        std::cerr << "Error: Could not write solutions.txt" << std::endl;
        return 1;
    }
    std::cout << "Saved to solutions.txt\n";
    //return the POSIX return code
    return 0;
}
