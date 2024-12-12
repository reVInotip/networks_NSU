using Avalonia.Controls;
using System;
using Dto;
using Avalonia.Interactivity;
using System.Collections.Generic;
using System.Net.Http;
using HtmlAgilityPack;
using Avalonia.Media;
using System.Text.RegularExpressions;
using Utils;
using System.Threading.Tasks;

namespace places.Views;

public partial class MainWindow : Window
{
    private readonly HttpClient imageLoader = new();
    private readonly PlacesService placesService;
    private readonly RegexWorker regexWorker;
    private readonly StackPanel interestingPlacesStack = new();

    public MainWindow()
    {
        InitializeComponent();
        placesService = new PlacesService();
        regexWorker = new(
            new Tuple<string, Regex>(
                "wiki",
                new Regex(
                    @"^https://commons.wikimedia.org/wiki.*",
                    RegexOptions.Compiled
                )
            ),
            new Tuple<string, Regex>(
                "allCulture",
                new Regex(
                    @"^https://all.culture.ru/uploads.*",
                    RegexOptions.Compiled
                )
            ),
            new Tuple<string, Regex>(
                "findImage",
                new Regex(
                    @"^https://upload.wikimedia.org.*",
                    RegexOptions.Compiled
                )
            )
        );
        imageLoader.DefaultRequestHeaders.Add("user-agent",
            "Mozilla/5.0 (Windows; Windows NT 5.1; rv:1.9.2.4) Gecko/20100611 Firefox/3.6.4");

        AddStatusTextBlock("Ready");
    }
    private async void ButtonOnClick(object? sender, RoutedEventArgs e)
    {
        ReadContainer.Children.Clear();
        var places = await placesService.GetPlaces(Search.Text);
        if (places.Count == 0)
        {
            AddPropertyTextBlock("Not found");
        }
        else
        {
            CreateButtons(places);
        }
        Search.Clear();
    }

    private async void CreateButtons(List<PlaceInfoDto> places)
    {
        ButtonsContainer.Children.Clear();
        AddStatusTextBlock("Search");

        foreach (var place in places)
        {
            Button button = Graphics.CreateButton(place.City == "unknown" ?
                                    $"Place: {place.Name} Kind: {place.Kind}" :
                                    $"Place: {place.Name} City: {place.City} Kind: {place.Kind}");
            await placesService.GetWeather(place);
            await placesService.GetInterestingPlaces(place);
            await placesService.GetDescription(place);

            SetPlaceButtonAction(button, place);

            ButtonsContainer.Children.Add(button);
        }

        AddStatusTextBlock("Completed");
    }

    private void SetPlaceButtonAction(Button placeButton, PlaceInfoDto place)
    {
        placeButton.Click += (sender, e) =>
        {
            {
                ReadContainer.Children.Clear();
                AddMainTextBlock(place.Name);
                AddPropertyTextBlock("Kind: " + place.Kind);
                AddPropertyTextBlock("City: " + place.City);
                AddPropertyTextBlock("Country: " + place.Country);
                AddPropertyTextBlock("Temperature: " + place.Temp + "°");
                AddPropertyTextBlock("Feels Like: " + place.Feels_like + "°");
                AddPropertyTextBlock("Pressure: " + place.Pressure + "mm Hg");
                AddPropertyTextBlock("Humidity: " + place.Humidity + "%");
                AddPropertyTextBlock("Latitude: " + place.Lat + "°");
                AddPropertyTextBlock("Longitude: " + place.Lng + "°");

                interestingPlacesStack.Children.Clear();

                Button showPlacesButton = Graphics.CreateButton("Show interesting places");
                showPlacesButton.Click += async (sender, e) =>
                {
                    interestingPlacesStack.Children.Clear();
                    foreach (var interestingPlace in place.InterestingPlaces)
                    {
                        DescriptionDto? descr = new();
                        place.Description.TryGetValue(interestingPlace.Name, out descr);
                        if (descr is not null)
                        {
                            AddInterestingPlaceTextBlock(interestingPlace.Name);

                            AddInterestingPlacePropertyTextBlock("Kinds: " + descr.Kinds);
                            AddInterestingPlacePropertyTextBlock("Distance: " + interestingPlace.Dist + " meters");
                            AddInterestingPlacePropertyTextBlock("Wiki: " + descr.Wiki);

                            if (descr.Image is not null)
                                await AddImage(descr.Image);
                        }
                        else
                        {
                            AddInterestingPlaceTextBlock(interestingPlace.Name);
                        }
                    }
                };

                Button hidePlacesButton = Graphics.CreateButton("Hide interesting places");
                hidePlacesButton.Click += (sender, e) =>
                {
                    interestingPlacesStack.Children.Clear();
                };

                ReadContainer.Children.Add(showPlacesButton);
                ReadContainer.Children.Add(hidePlacesButton);
                ReadContainer.Children.Add(interestingPlacesStack);
            };
        };
    }

    private async Task AddImage(string image)
    {
        if (regexWorker.Compare("wiki", image))
        {
            var document = await new HtmlWeb().LoadFromWebAsync(image);

            List<HtmlNode> imageNodes = [.. document.DocumentNode.SelectNodes("//img")];
            if (imageNodes.Count != 0)
            {
                foreach (HtmlNode imageNode in imageNodes)
                {
                    if (
                        regexWorker.Compare("findImage", imageNode.Attributes["src"].Value) &&
                        imageNode.Attributes["alt"].Value.Contains("File:")
                    )
                    {
                        var message = await imageLoader.GetAsync(imageNode.Attributes["src"].Value);
                        var img_response = await message.Content.ReadAsByteArrayAsync();
                        if (img_response is not null)
                        {
                            interestingPlacesStack.Children.Add(
                                CreateImagePanel(img_response)
                            );
                        }

                        break;
                    }
                }
            }
        }
        else if (regexWorker.Compare("allCulture", image))
        {
            var message = await imageLoader.GetAsync(image);
            var img_response = await message.Content.ReadAsByteArrayAsync();
            if (img_response is not null)
            {
                interestingPlacesStack.Children.Add(
                    CreateImagePanel(img_response)
                );
            }
        }
    }

    private void AddStatusTextBlock(string status)
    {
        StatusPanel.Children.Clear();
        StatusPanel.Children.Add(Graphics.CreateStatusTextBlock(status));
    }

    private void AddPropertyTextBlock(string text) =>
        ReadContainer.Children.Add(Graphics.CreatePlacePropertiesTextBlock(text));

    private void AddMainTextBlock(string text) =>
        ReadContainer.Children.Add(Graphics.CreateMainTextBlock(text));

    private void AddInterestingPlaceTextBlock(string text) =>
        interestingPlacesStack.Children.Add(Graphics.CreateInterestingPlaceTextBlock(text));

    private void AddInterestingPlacePropertyTextBlock(string text) =>
        interestingPlacesStack.Children.Add(Graphics.CreateInterestingPlacePropertyTextBlock(text));

    private Panel CreateImagePanel(byte[] imageSource) =>
        Graphics.CreateImagePanel(imageSource, Bounds.Width / 1.5, Bounds.Height / 2.5 - 5);
}