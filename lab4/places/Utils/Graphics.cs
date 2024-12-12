using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;

namespace Utils;

public class Graphics
{
    private static Button emptyButton = new();

    public static Button CreateButton(string text)
    {
        TextBlock buttonText = new()
        {
            TextWrapping = TextWrapping.Wrap,
            Text = text,
        };

        return new Button
        {
            Content = buttonText,
            Foreground = Brushes.Black,
            Margin = new Thickness(0, 0, 0, 5)
        };
    }

    public static Panel CreateImagePanel(byte[] imageSource, double width, double height)
    {
        Panel panel = new();
        panel.Children.Add(new Border
        {
            Width = width,
            Height = height,
            BorderBrush = Brushes.DarkGray,
            Background = Brushes.LightGray,
            BorderThickness = Thickness.Parse("2")
        });

        panel.Children.Add(new Image
        {
            Source = new Bitmap(new MemoryStream(imageSource)),
            MaxHeight = height,
            MaxWidth = width
        });

        return panel;
    }

    public static TextBlock CreatePlacePropertiesTextBlock(string text)
    {
        return new TextBlock
        {
            Foreground = Brushes.Black,
            TextWrapping = TextWrapping.Wrap,
            Text = text,
            FontWeight = FontWeight.Bold
        };
    }

    public static TextBlock CreateMainTextBlock(string text)
    {
        return new TextBlock
        {
            TextWrapping = TextWrapping.Wrap,
            Text = "Place: " + text,
            FontSize = 20,
            FontWeight = FontWeight.Bold,
            Foreground = Brushes.Black
        };
    }

    public static TextBlock CreateInterestingPlaceTextBlock(string text)
    {
        return new TextBlock
        {
            TextWrapping = TextWrapping.Wrap,
            Text = text + ":",
            FontWeight = FontWeight.Bold,
            FontSize = 14,
            Foreground = Brushes.Black
        };
    }
    public static TextBlock CreateInterestingPlacePropertyTextBlock(string text)
    {
        return new TextBlock
        {
            TextWrapping = TextWrapping.Wrap,
            Text = text,
            Foreground = Brushes.Black,
            Padding = new Thickness(5, 0, 5, 0)
        };
    }

    public static TextBlock CreateStatusTextBlock(string text)
    {
        return new TextBlock
        {
            Text = "STATUS:" + text,
            FontSize = 14,
            Foreground = Brushes.Black,
            Padding = new Thickness(5, 5, 5, 5),
            FontWeight = FontWeight.Bold
        };
    }
}