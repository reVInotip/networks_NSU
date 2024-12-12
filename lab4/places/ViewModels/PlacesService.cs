using System;
using PlacesApi.Location;
using PlacesApi.Description;
using PlacesApi.InterestingPlaces;
using PlacesApi.Weather;
using Dto;
using System.Threading.Tasks;
using System.Collections.Generic;
using Utils;

class PlacesService
{
    private readonly ApiWorker apiWorker;

    public PlacesService() => apiWorker = new ApiWorker();

    public async Task<List<PlaceInfoDto>> GetPlaces(string? name)
    {
        if (name is null) return [];

        var places = new List<PlaceInfoDto>();
        var json = (LocationJson?)await apiWorker.GetLocations(name);
        if (json is not null && json.hits is not null)
        {
            foreach (var place in json.hits)
            {
                if (place.name != "")
                {
                    var placeInfo = new PlaceInfoDto
                    {
                        Name = place.name,
                        City = place.city,
                        Country = place.country,
                        Lat = place.point.lat,
                        Lng = place.point.lng,
                        Kind = place.osm_value
                    };
                    places.Add(placeInfo);
                }

            }
        }
        return places;
    }

    public async Task GetDescription(PlaceInfoDto placeInfo)
    {
        for (int i = 0; i < placeInfo.InterestingPlaces.Count; i++)
        {
            var json = (DescriptionJson?)await apiWorker.GetDescription(placeInfo.IdInterestingPlaces[i]);

            if (json is not null)
            {
                placeInfo.Description[placeInfo.InterestingPlaces[i].Name] = new DescriptionDto
                {
                    Kinds = json.kinds,
                    Info = json.info.descr,
                    Wiki = json.wikipedia_extracts.text,
                    Image = json.image
                };
            }
        }
    }

    public async Task GetInterestingPlaces(PlaceInfoDto placeInfo)
    {
        var json = (InterestingPlacesJson?)await apiWorker.GetInterestingPlaces(placeInfo.Lat, placeInfo.Lng);
        if (json is not null && json.features is not null)
        {
            foreach (var feature in json.features)
            {
                if (feature.properties is not null && feature.properties.name is not null && feature.properties.name != "")
                {
                    placeInfo.AddInterestingPlaces(
                        new InterestingPlaceDto
                        {
                            Name = feature.properties.name,
                            Dist = feature.properties.dist
                        },
                        feature.properties.xid);
                }
            }
        }
    }

    public async Task GetWeather(PlaceInfoDto placeInfo)
    {
        var json = (WeatherJson?)await apiWorker.GetWeather(placeInfo.Lat, placeInfo.Lng);
        if (json is not null && json.main is not null)
        {
            placeInfo.AddWeather(Converter.TemperatureInKelvinToCelsius(json.main.temp).ToString(),
                                Converter.PressureInHPaToMilimetersOfMercury(json.main.pressure).ToString(),
                                json.main.humidity.ToString(),
                                Converter.TemperatureInKelvinToCelsius(json.main.feels_like).ToString());
        }
    }
}