using System;
using System.Net.Http;
using System.Threading.Tasks;
using PlacesApi;
using PlacesApi.Description;
using PlacesApi.InterestingPlaces;
using PlacesApi.Location;
using PlacesApi.Weather;

public class ApiWorker
{
    private readonly HttpClient httpClient = new();
    private readonly WeatherApi weatherApi;
    private readonly LocationApi locationApi;
    private readonly DescriptionApi descriptionApi;
    private readonly InterestingPlacesApi interestingPlacesApi;

    public ApiWorker()
    {
        weatherApi = new WeatherApi();
        descriptionApi = new DescriptionApi();
        interestingPlacesApi = new InterestingPlacesApi();
        locationApi = new LocationApi();
    }

    public async Task<InformJson>? GetLocations(string nameLocation)
    {
        return await locationApi.GetResult(nameLocation, httpClient);
    }

    public async Task<InformJson>? GetWeather(double lat, double lon)
    {
        return await weatherApi.GetResult(lon.ToString() + ";" + lat.ToString(), httpClient);
    }

    public async Task<InformJson>? GetInterestingPlaces(double lat, double lon)
    {
        return await interestingPlacesApi.GetResult(lon.ToString() + ";" + lat.ToString(), httpClient);
    }

    public async Task<InformJson>? GetDescription(string id)
    {
        return await descriptionApi.GetResult(id, httpClient);
    }
}