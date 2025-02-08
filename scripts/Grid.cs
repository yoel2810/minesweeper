using Godot;
using System;

public partial class Grid : Node2D
{
    private int rows;
    private int cols;
    private int numMines;
    private Cell[,] cells;
    private bool isGameOver;
    private int cellsRevealed;
    private Vector2 shiftGridLocation;
    private readonly double DIFFICULTY = 22;


    public override void _Ready()
    {
        rows = 19;
        cols = 35;
        numMines = (int)Math.Floor(rows * cols * (DIFFICULTY / 100));
        isGameOver = false;
        cellsRevealed = 0;
        cells = new Cell[rows, cols];
        shiftGridLocation = new Vector2(Cell.CELL_SIZE / 2, -Cell.CELL_SIZE / 2) + GetCenterShift();
        GenerateGrid();
    }

    private Vector2 GetCenterShift()
    {
        Vector2 viewportSize = GetViewportRect().Size;
        Vector2 gridSize = new Vector2(cols * Cell.CELL_SIZE, rows * Cell.CELL_SIZE);
        Vector2 centeredPosition = (viewportSize - gridSize) / 2;
        return centeredPosition;
    }

    private void GenerateGrid()
    {
        Position += shiftGridLocation;
        PackedScene cellScene = ResourceLoader.Load<PackedScene>("res://scenes/Cell.tscn");

        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                Cell cell = cellScene.Instantiate<Cell>();
                AddChild(cell);
                cells[row, col] = cell;
                cell.Position = new Vector2(col * Cell.CELL_SIZE, row * Cell.CELL_SIZE + Cell.CELL_SIZE);
            }
        }
    }

    private void PlaceMines(int forbiddenRow = -1, int forbiddenCol = -1)
    {
        Random random = new Random();
        int minesPlaced = 0;
        while (minesPlaced < numMines)
        {
            int row = random.Next(rows);
            int col = random.Next(cols);
            if (!cells[row, col].IsMine && row != forbiddenRow && col != forbiddenCol)
            {
                cells[row, col].IsMine = true;
                minesPlaced++;
            }
        }
    }

    private bool IsValidCell(int row, int col)
    {
        return row >= 0 && row < rows && col >= 0 && col < cols;
    }

    private void CalculateAdjaceny()
    {
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                if (cells[row, col].IsMine)
                    continue;

                int adjacentMines = 0;
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        if (IsValidCell(row + i, col + j) && cells[row + i, col + j].IsMine)
                            adjacentMines++;
                    }
                }
                cells[row, col].AdjacentMines = adjacentMines;
            }
        }
    }

    private void RevealCell(int row, int col)
    {
        cellsRevealed++;
        if (!IsValidCell(row, col) || cells[row, col].IsRevealed)
            return;

        if (cellsRevealed == 1)
        {
            PlaceMines(row, col);
            CalculateAdjaceny();
        }

        cells[row, col].Reveal();

        if (cells[row, col].IsMine)
        {
            isGameOver = true;
            return;
        }

        if (cells[row, col].AdjacentMines == 0 || cellsRevealed == 1)
        {

            for (int i = -1; i <= 1; i++)
            {
                for (int j = -1; j <= 1; j++)
                {
                    if (IsValidCell(row + i, col + j) && !cells[row + i, col + j].IsMine && !cells[row + i, col + j].IsFlagged)
                        RevealCell(row + i, col + j);
                }
            }
        }
    }

    private void ToggleFlag(int row, int col)
    {
        if (IsValidCell(row, col) && !cells[row, col].IsRevealed)
        {
            cells[row, col].ToggleFlag();
        }
    }

    private bool CheckWin()
    {
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                if (cells[row, col].IsFlagged && !cells[row, col].IsMine)
                {
                    return false;
                }

                if (!cells[row, col].IsRevealed && !cells[row, col].IsFlagged)
                {
                    return false;
                }

                if (!cells[row, col].IsRevealed && !cells[row, col].IsMine)
                {
                    return false;
                }
            }
        }
        return true;
    }

    private bool IsGameOver()
    {
        return isGameOver;
    }

    private Cell[,] GetCells()
    {
        return cells;
    }


    public override void _Input(InputEvent @event)
    {
        if (IsGameOver())
            return;

        if (@event is InputEventMouseButton mouseEvent && @event.IsPressed())
        {
            Vector2 mousePosition = ToLocal(mouseEvent.Position) + new Vector2(Cell.CELL_SIZE / 2, -Cell.CELL_SIZE / 2);
            int row = (int)Math.Floor(mousePosition.Y / Cell.CELL_SIZE);
            int col = (int)Math.Floor(mousePosition.X / Cell.CELL_SIZE);
            if (!IsValidCell(row, col))
                return;
            if (mouseEvent.ButtonIndex == MouseButton.Left)
            {
                if (cells[row, col].IsFlagged)
                {
                    return;
                }

                // if the cell is reveales and the number for flagged cells around it is equal to the number of mines around it, reveal all adjacent cells
                int flaggedCells = 0;
                for (int i = -1; i <= 1; i++)
                {
                    for (int j = -1; j <= 1; j++)
                    {
                        if (IsValidCell(row + i, col + j) && cells[row + i, col + j].IsFlagged)
                            flaggedCells++;
                    }
                }
                if (flaggedCells == cells[row, col].AdjacentMines && cells[row, col].IsRevealed)
                {
                    for (int i = -1; i <= 1; i++)
                    {
                        for (int j = -1; j <= 1; j++)
                        {
                            if (IsValidCell(row + i, col + j) && !cells[row + i, col + j].IsFlagged)
                                RevealCell(row + i, col + j);
                        }
                    }
                }
                else
                    RevealCell(row, col);

                if (IsGameOver())
                {
                    ShowGameOverUI();
                    return;
                }

            }
            else if (mouseEvent.ButtonIndex == MouseButton.Right)
            {
                ToggleFlag(row, col);
            }

            if (CheckWin())
            {
                ShowWinUI();
            }
        }
    }

    private void ShowGameOverUI()
    {
        PackedScene gameOverScene = ResourceLoader.Load<PackedScene>("res://scenes/GameOverUI.tscn");
        Control gameOverUI = gameOverScene.Instantiate<Control>();
        AddChild(gameOverUI);
    }

    private void ShowWinUI()
    {
        PackedScene winScene = ResourceLoader.Load<PackedScene>("res://scenes/WinningUI.tscn");
        Control winUI = winScene.Instantiate<Control>();
        AddChild(winUI);
    }
}
