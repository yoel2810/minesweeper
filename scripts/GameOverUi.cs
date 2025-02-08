using Godot;
using System;

public partial class GameOverUi : Control
{
	public override void _Ready()
	{
		GetNode<Button>("Button").Connect("pressed", new Callable(this, nameof(OnButtonPressed)));
	}

	private void OnButtonPressed()
	{
		GetTree().ChangeSceneToFile("res://scenes/Grid.tscn");
	}
}
