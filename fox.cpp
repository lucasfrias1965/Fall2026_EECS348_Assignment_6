/*
	<@
	(KU/
	 "
human.cpp - by google gemini flash
modified slightly by lucas frias
 Program:       EECS 348 Assignment 5 - Sudoku solver (Assignment5.cpp, revised from human.cpp)
 Description:  Solves Sudoku problems by depth-first search with backtracking and finds EVERY solution.
               Each solution is printed to the console and also saved to solutionN.txt (for puzzleN.txt).
               Assignment 5 revision: digits are tracked in bitmasks, the empty cell with the fewest
               legal digits is filled first, and contradictory givens are detected while loading.
 Input:         puzzle1.txt ... puzzle5.txt (9 rows of 9 cells, '_' = empty cell, 1-9 = given digit)
 Output:        Console report for every puzzle, plus solution1.txt ... solution5.txt. A solution file holds
               either all solution grids (same layout as the puzzle files, one blank line after each grid)
               followed by the single line "Solutions found: n", or just the single line "No solution found."
 Collaborators:  Google Gemini Flash, Lucas Frias, Claude (Anthropic)
 Other sources:  Gemini Flash wrote the Assignment 4 base; Claude (Anthropic) wrote the Assignment 5 changes
 Block tags:     [A5-Claude]     new code added in Assignment 5, written by Claude and reviewed by the reviser
                 [A4+A5-Claude]  Assignment 4 code that Claude changed in Assignment 5
                 (no tag)        Assignment 4 code kept exactly as received (Gemini Flash / Lucas Frias)
 * Author:        Lucas Frias (by using Gemini Flash)
 * Created:       October 13th 2026 
 * Revised:        2026-10-13, implemented fix features 
 *                category to enum class Category, per Opus 5.5 pairing
 * Revised by:    Jacob Fox, 2026-10-26 (Assignment 5: solution files, faster search, input checks)
 * Build:         g++ Assignment5.cpp -std=c++11 -Wall -Wextra -o Assignment5.exe
 * Run:           ./Assignment5.exe (puzzle1.txt through puzzle5.txt must be in the same folder)
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
    // unused (0x3FE when SIZE is 9). Sets of candidate digits are stored as masks like this one
    static const unsigned int ALL_DIGITS = (1u << (SIZE + 1)) - 2;
    //make a grid with our set size (always a square)
    int grid[SIZE][SIZE];
    // [A5-Claude] bookkeeping for the Sudoku rules. Bit d of rowUsed[r] is on when digit d is already in row r;
    // colUsed and boxUsed do the same for columns and 3x3 boxes. A digit can then be tested with one AND
    // instead of scanning 27 cells. The "= {}" starts every mask at zero (no digits used yet)
    unsigned int rowUsed[SIZE] = {};
    unsigned int colUsed[SIZE] = {};
    unsigned int boxUsed[SIZE] = {};
    // [A5-Claude] turns true if two given digits break a Sudoku rule, which means no solution can exist
    bool givensConflict = false;
    //make a filename
    std::string filename;
    // [A5-Claude] name of the file that receives this puzzle's answers (puzzle3.txt -> solution3.txt)
    std::string solutionFilename;
    // [A5-Claude] stream for that file. It stays open during the search so every answer is written the
    // moment it is found, which means no solution is ever kept in memory
    std::ofstream solutionFile;
    //make a counter for the solutions found
    //lucas edit, the solutionsFound should be zero
    //initialised when declared
    int solutionsFound = 0;

    // Checks if placing 'num' at grid[row][col] is valid

    bool isValid(int row, int col, int num) const {
        // [A4+A5-Claude] the old body scanned the row, column and box (27 cells) on every try. Now it is a
        // single bit test: candidates() already knows which digits are legal here, so shift that mask right by
        // 'num' and look at the lowest bit (1 means digit 'num' is legal in this cell)
        return ((candidates(row, col) >> num) & 1u) != 0;
    }

    // [A5-Claude] Tells which of the nine 3x3 boxes a cell sits in. Boxes are numbered 0..8, left to right and
    // then top to bottom, so the centre box is number 4. It picks the right entry of boxUsed
    int boxOf(int row, int col) const {
        // row / 3 gives the band of boxes (0, 1 or 2); col / 3 gives the box inside that band
        return (row / BLOCK_SIZE) * BLOCK_SIZE + (col / BLOCK_SIZE);
    }

    // [A5-Claude] Returns the digits that may still legally go into cell (row, col), one bit per digit.
    // This is the only place where the row, column and box rules of Sudoku are checked
    unsigned int candidates(int row, int col) const {
        // OR the three "already used" masks together: a 1 bit means that digit is blocked for this cell
        unsigned int blocked = rowUsed[row] | colUsed[col] | boxUsed[boxOf(row, col)];
        // keep only the digit bits (1..9) that are not blocked
        return ALL_DIGITS & ~blocked;
    }

    // [A5-Claude] Counts the 1 bits in a mask, i.e. how many candidate digits a cell has left
    static int countBits(unsigned int mask) {
        int count = 0;          // running total of 1 bits seen so far
        while (mask != 0) {     // keep going until no 1 bits are left
            mask &= mask - 1;   // switch off the lowest 1 bit
            count++;            // and count it
        }
        return count;           // 0..9 for our masks
    }

    // [A5-Claude] Puts a digit into a cell and records it in the row, column and box masks
    void place(int row, int col, int num) {
        const unsigned int bit = 1u << num;   // one-hot mask of this digit (digit 5 -> binary 100000)
        grid[row][col] = num;                 // write the digit into the grid
        rowUsed[row] |= bit;                  // the digit is now used in this row
        colUsed[col] |= bit;                  // ... in this column
        boxUsed[boxOf(row, col)] |= bit;      // ... and in this box
    }

    // [A5-Claude] Exact reverse of place(): empties the cell and frees its digit in the three masks
    void unplace(int row, int col) {
        const unsigned int bit = 1u << grid[row][col];  // which digit is being taken back out
        rowUsed[row] &= ~bit;                           // free it in this row
        colUsed[col] &= ~bit;                           // ... in this column
        boxUsed[boxOf(row, col)] &= ~bit;               // ... and in this box
        grid[row][col] = 0;                             // the cell is empty again
    }

    // [A5-Claude] Runs once for every completed grid the search reaches. Keeping this out of solveRecursive
    // means the search code only searches, and all the output work for a solution lives in one place
    void recordSolution() {
        solutionsFound++;                                       // one more solution
        std::cout << "\nSolution " << solutionsFound << ":\n";  // console label, same as Assignment 4
        printGrid(std::cout);                                   // console copy of the grid
        printGrid(solutionFile);                                // file copy, same layout as the puzzle files
        solutionFile << "\n";                                   // blank line so grids in the file stay apart
    }

    // Recursive helper to find all solutions using depth-first search with backtracking
    void solveRecursive() {
	// gets the recursive solution
        // [A4+A5-Claude] no row/col arguments any more. Instead of walking the grid left to right, every call
        // looks for the empty cell with the FEWEST legal digits and branches on that one. A cell with one
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
            // A cell with zero candidates proves this branch cannot work.
            if (count == 0) {
                bestRow = row;
                bestCol = col;
                bestMask = 0;
                bestCount = 0;
                break;
            }

            // Otherwise, remember the cell with the fewest candidates.
            if (count < bestCount) {
                bestRow = row;
                bestCol = col;
                bestMask = mask;
                bestCount = count;
            }
        }
    

        // no empty cell was found, so the grid is full. Every digit went in through the masks, so it is a
        // valid solution
        if (bestRow == -1) {
            recordSolution();       // count it, print it and save it
            return;                 // back out and keep looking for other solutions
        }

        // Branch on the chosen cell, trying its legal digits in ascending order. If it has no legal digits,
        // bestMask is 0, no digit passes the test below and the loop does nothing, so a dead end simply ends
        // this branch with no special case
	//see canidates 1-9 on our current element
        for (int num = 1; num <= SIZE; num++) {
            if ((bestMask >> num) & 1u) {         // [A4+A5-Claude] is digit 'num' one of this cell's legal digits?
                place(bestRow, bestCol, num);     // put the digit down and update the masks
                solveRecursive();                 // search the rest of the grid
                unplace(bestRow, bestCol);        // Backtrack: take it out again and try the next digit
            }
        }
    }

public:
	//initialiser method, takes a sting and requires a filename
	//calculates the solutionsFound
    // [A4+A5-Claude] now also receives the name of the solution file this puzzle writes to
    SudokuSolver(const std::string& file, const std::string& outFile) : filename(file), solutionFilename(outFile), solutionsFound(0) {
        for (int row = 0; row < SIZE; row++) {
            for (int col = 0; col < SIZE; col++) {
		//zero intiialise all data
                grid[row][col] = 0;
            }
        }
    }

    // Reads the grid from the file
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
                    // [A4+A5-Claude] a given digit. If its row, column or box already holds the same digit, two
                    // givens clash and no solution can exist. Either way the digit is stored and added to the masks
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
        // [A5-Claude] all 81 cells are read. If another non-blank character is still in the file it is not a
        // 9x9 puzzle (a 10th column would silently shift every row), so refuse the file instead of ignoring it
        if (file >> cell) {
            std::cerr << "Error: Extra data after the 81st cell in " << filename << std::endl;
            return false;
        }
	//load puzzle returns a boolean, which is kinda weird but not 
	//too bad of a choice. it makes sense to do the init method
        return true;
    }

    // Prints the grid
    // [A4+A5-Claude] the output stream is now a parameter, so one function prints to the console and to the file
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

    // Primary object method to run solving routine and output formatted results
    void solve() {
	//display the filename and the puzzle
        std::cout << "========================================\n";
        std::cout << "File: " << filename << "\n";
        std::cout << "----------------------------------------\n";
        std::cout << "Initial Puzzle:\n";
        printGrid(std::cout);
        // [A5-Claude] create (or empty) this puzzle's solution file so answers can be written as they are found
        solutionFile.open(solutionFilename);

        if (!solutionFile.is_open()) {
            std::cerr << "Error: Could not create output file "
                    << solutionFilename << '\n';
            return;
        }

	//here we print all solutions by just recursively
	//going and display stuff. solutionsFound is a public
	//var so we have space for it.
        solutionsFound = 0;
        // [A4+A5-Claude] if two givens clash there is nothing to search; otherwise run the search
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
        // [A5-Claude] close the file (this also flushes it), then tell the user whether the save worked
        solutionFile.close();
        if (solutionFile.fail()) {
            std::cerr << "Error: Could not write " << solutionFilename << std::endl;
        } else {
            std::cout << "Saved to " << solutionFilename << "\n";
        }
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
    // [A5-Claude] counts the puzzles as the loop reaches them (1 for puzzle1.txt, 2 for puzzle2.txt, ...), so
    // puzzle N saves its answers to solutionN.txt. Keep puzzleFiles in numeric order for this to hold
    int puzzleNumber = 0;
    //use dynamic iteration if we wanna add more puzzles
    for (const auto& filename : puzzleFiles) {
        puzzleNumber++;   // [A5-Claude] move on to the next puzzle number
	//make a sudoku instance
        SudokuSolver solver(filename, "solution" + std::to_string(puzzleNumber) + ".txt");   // [A4+A5-Claude] also pass solutionN.txt
	//can we load it?
        if (solver.loadPuzzle()) {
            solver.solve();//then solve it, which prints everything
        }
    }
    //return the POSIX return code
    return 0;
}

