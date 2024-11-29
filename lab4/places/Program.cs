using Avalonia;
using System;
using System.Net.Http;
using System.Threading.Tasks;

namespace places;

sealed class Program
{
    // Initialization code. Don't use any Avalonia, third-party APIs or any
    // SynchronizationContext-reliant code before AppMain is called: things aren't initialized
    // yet and stuff might break.
    [STAThread]
    public static async Task Main(string[] args) {
        System.Net.Http.HttpClient client = new();

        using HttpResponseMessage request = await client.GetAsync("https://graphhopper.com/api/1/geocode?q=string&locale=en&limit=5&reverse=false&debug=false&point=string&provider=default&key=YOUR_API_KEY_HERE");
        string response = await request.Content.ReadAsStringAsync();

        Console.WriteLine(response);

        BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
    }

    // Avalonia configuration, don't remove; also used by visual designer.
    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .WithInterFont()
            .LogToTrace();
}
