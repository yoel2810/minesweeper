using Godot;
using System;

public partial class Cell : Node2D
{
    public bool IsMine { get; set; }
    public bool IsRevealed { get; set; }
    public bool IsFlagged { get; set; }
    public int AdjacentMines { get; set; }
    private Sprite2D sprite;
    public const int CELL_SIZE = 32;

    [Export]
    public Texture2D[] textures = new Texture2D[12];

    public override void _Ready()
    {
        sprite = GetNode<Sprite2D>("Sprite2D");
        if (sprite == null)
        {
            GD.PrintErr("Sprite2D node not found!");
        }
    }

    public void Reveal()
    {
        IsRevealed = true;
        if (IsMine)
        {
            sprite.Texture = textures[11];
        }
        else
        {
            sprite.Texture = textures[AdjacentMines];
        }
    }

    public void ToggleFlag()
    {
        if (IsFlagged)
        {
            sprite.Texture = textures[9];
        }
        else
        {
            sprite.Texture = textures[10];
        }
        IsFlagged = !IsFlagged;
    }

}

