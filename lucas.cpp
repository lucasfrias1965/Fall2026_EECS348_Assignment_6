////////////////////////////////////////
/*
        <@
        (KU//
          "
    EECS 348 - Software Engineering
    Assignment 5
    By Lucas Scott Frias
    Description: This program, given the
    input files:
        puzzle1.txt, puzzle2.txt, puzzle3.txt
        puzzle4.txt, puzzle5.txt
    exist within the current working directory,
    will output their solutions to a file solutionN.txt
    or, if no solutions exist, output there are no solutions.
    Collaborators:
        - Claude Opus 5.5 -> used specification rewrite
        - Flash -> generated base code
        - Mr. Andy Garcia of Sofware Engineering -> wrote comments and wrote some code

    Creation Date: 10/28/26 (the day I wrote this comment)

*/


#include <iostream> // Include input-output stream library for console printing
#include <fstream>  // Include file stream library for reading puzzle text files
#include <vector>   // Include vector library to store the 9x9 grid and list of solutions
#include <string>   // Include string library for handling file names and strings

class SudokuSolver
{                                        // Creates a class to make a SudokuSolver
private:                                 // Make it private instead of public
    std::vector<std::vector<int>> board; // Use a 2d vector to represent the grid of size 9x9

    bool isValid(int row, int col, int num)   // Source: Gemini with unchanged logic; Comments by Andy Garcia
    { // Method used to check if a digit can be placed at given index by looking through rows, columns, and 3x3 grids
        for (int i = 0; i < 9; i++)
        { // Loop through every column in the given row
            if (board[row][i] == num)
            {
                return false; // Return false if the number is already in the row
            }
        }
        for (int i = 0; i < 9; i++)
        { // Loop through each row in the current column
            if (board[i][col] == num)
            {
                return false; // Return false if the number is already in the col
            }
        }
        int startRow = row - row % 3; // Find the starting row index of the 3x3 grid
        int startCol = col - col % 3; // Find the starting column index of the 3x3 subgrid
        for (int r = 0; r < 3; r++)
        { // Loop through all 3 rows of the subgrid
            for (int c = 0; c < 3; c++)
            { // Loop through all 3 columns of the subgrid
                if (board[startRow + r][startCol + c] == num)
                {
                    return false; // Return false if it is found
                }
            }
        }
        return true; // Return true if the placement is valid across row, col, and subgrid
    }

    // Function authored by Andy Garcia to detect if a digit is repeated on a row or column. Error detected by Claude and given assistance with logic and code implemented by Andy Garcia
    // Renamed from givensAreValid to givensHaveNoDuplicates to state precisely what the check does
    bool givensHaveNoDuplicates()
    {                               // new function to test if duplicate nums exist on the same row or col
        for (int r = 0; r < 9; r++) // loop through every row
            for (int c = 0; c < 9; c++)
            {                        // loop through every col
                int d = board[r][c]; // set d equal to the digit at the board spot looked at
                if (d == 0) continue; // skip blanks
                board[r][c] = 0;            // take it off the board
                bool ok = isValid(r, c, d); // does it conflict with the rest?
                board[r][c] = d;            // put it back
                if (!ok)
                {
                    return false; // two givens clash
                }
            }
        return true; // otherwise return true
    }

    int solutionCount = 0; // new private member added to count the number of solutions done

    // Code Change #3 : edited Gemini's code to stop storing a solution each time it was finished and instead print it off then clear it. Storage fix discovered by Claude and code implemented by Andy Garcia
    void solveRec(int row, int col, std::ostream &output)  // Write each solution to the output file
    { // Recursive backtracking method. Removed solutions parameter (Andy Garcia)
        if (row == 9)
        {                                                       // Check if we have successfully processed past the last row
            solutionCount++;                                    // count it
            output << "Solution " << solutionCount << ":\n"; // Label each solution
            printBoard(output);                                  // Write the solved board
            output << "\n";                                     // Separate solutions
            return;                                             // ends the function
        }
        int nextRow = (col == 8) ? row + 1 : row; // Calculate the row index for the next step of the recursion
        int nextCol = (col == 8) ? 0 : col + 1;   // Calculate the column index for the recusion next step

        if (board[row][col] != 0)
        {                               // Checks if cell is already filled
            solveRec(nextRow, nextCol, output); // Go to the next cell without changing the current
        }
        else
        { // If the current cell is blank (represented by 0)
            for (int num = 1; num <= 9; num++)
            { // Try digits 1 through 9 sequentially
                if (isValid(row, col, num))
                {                               // Check if the digit in the loop is valid in that space
                    board[row][col] = num;      // Temporarily place it on the board
                    solveRec(nextRow, nextCol, output); // Use recursion to solve the next cell
                    board[row][col] = 0;        // Then backtrack the cell to be a 0 which represent blank
                }
            }
        }
    }

public: // Public interface for the SudokuSolver class
    SudokuSolver(const std::vector<std::vector<int>> &initialBoard)
    {                         // Constructor taking initial board
        board = initialBoard; // Initialize the board by using the provided puzzle
    }

    int findSolutions(std::ostream &output)  // Find and write all solutions
    { // Public method to trigger solving. Updated to take no parameters and return an int of how many solutions found (Andy Garcia)
        if (!givensHaveNoDuplicates())
        {
            return 0; // uses new method to quit and have no solution if a duplicate is detected (Andy Garcia)
        }
        solveRec(0, 0, output); // Start the recursion from the first cell
        return solutionCount; // returns the solution count when findSolutions is called
    }

    void printBoard(std::ostream &output = std::cout) const   // Print the current board to a stream
    { // Method to print the current state of the board
        for (int r = 0; r < 9; r++)
        { // Loop through each row of the board
            for (int c = 0; c < 9; c++)
            { // Loop through each column of the row
                if (board[r][c] == 0)
                {                      // Check if the cell value is 0 (blank)
                    output << "_ "; // Print an underscore for blank cells
                }
                else
                {                                    // If the cell contains a valid digit
                    output << board[r][c] << " "; // Print the digit followed by a space
                }
            }
            output << "\n"; // Print a newline at the end of each row
        }
    }
};

class PuzzleManager
{       // Build a class to run the program and read the files
public: // These are the public methods for our file management
    void processPuzzle(const std::string &filename)  // source Gemini; comments by Andy Garcia
    {
        std::ifstream infile(filename); // Open the file

        std::cout << "File: " << filename << "\n"; // Print the current file name
                if (!infile.is_open())
        {                                                                     // Check if the file failed to open
            std::cout << "Error: Could not open file " << filename << "\n\n"; // Print out the error message
            return;                                                           // Move to the next file
        }

        std::vector<std::vector<int>> initialBoard(9, std::vector<int>(9, 0)); // Create 9x9 grid with all zeros
        // Code Change #4 : removed the readSuccess flag and nested breaks, erroring and returning immediately
        // the moment a malformed character or short read is found (Idea presented by Claude, code written by Andy Garcia)
        for (int r = 0; r < 9; r++)
        { // Go through the 9 rows of the grid
            for (int c = 0; c < 9; c++)
            {            // Go through all 9 columns in each row
                char ch; // Use a variable to hold the character read in the file
                if (!(infile >> ch))
                {                                                                   // Try to read the file from the file stream
                    infile.close();                                                 // Close the file stream safely
                    std::cout << "Error: Invalid puzzle in " << filename << "\n\n"; // will now print an error message when a file is not correct. (Andy Garcia)
                    return;                                                         // returns to move on to next file after detecting bad one. (Andy Garcia)
                }
                if (ch == '_')
                {                           // Check if the character is a blank cell
                    initialBoard[r][c] = 0; // Write a 0 to represent blank cell on our board
                }
                else if (ch >= '1' && ch <= '9')
                {                                  // Check if character is a digit 1 through 9
                    initialBoard[r][c] = ch - '0'; // Change that character to an int on our board
                }
                else
                {                                                                   // Handle unexpected characters in the file
                    infile.close();                                                 // Close the file stream safely
                    std::cout << "Error: Invalid puzzle in " << filename << "\n\n"; // CODE CHANGE #1 : This detects a bad character and errors out right away (Idea presented by Claude, code written and commented by Andy Garcia)
                    return;                                                         // leaves the function immediately on bad input
                }
            }
        }

        infile.close(); // Close the file stream safely

        // Convert puzzleN.txt to solutionN.txt.
        std::string solutionFilename = filename;
        solutionFilename.replace(0, std::string("puzzle").size(), "solution");
        std::ofstream solutionFile(solutionFilename);
        if (!solutionFile.is_open())
        {
            std::cout << "Error: Could not create file " << solutionFilename << "\n\n";
            return;
        }

        SudokuSolver solver(initialBoard);
        int count = solver.findSolutions(solutionFile);
        if (count == 0)
        {
            solutionFile << "No solution found\n";
            std::cout << "No solution found. See " << solutionFilename << "\n\n";
        }
        else
        {
            solutionFile << "Solutions found: " << count << "\n";
            std::cout << "Solutions found: " << count << ". See " << solutionFilename << "\n\n";
        }
        solutionFile.close();
    }

    void run()
    { // Main function to run through all 5 files
        for (int fileIndex = 1; fileIndex <= 5; fileIndex++)
        {                                                                         // Loops from 1 to 5 to reach all files
            std::string filename = "puzzle" + std::to_string(fileIndex) + ".txt"; // build the name of the files in a string
            processPuzzle(filename);                                              // call new function to shrink run and improve maintainability (Andy Garcia)
        }
    }
};

int main()
{                          // Main function of the program
    PuzzleManager manager; // Creates instance of the PuzzleManager class
    manager.run();         // Start run() to begin the process
    return 0;              // end the program by returning 0
}
