using System.Collections.Generic;

namespace Dto {
    public class PlaceInfoDto {
        public string Temp { get; set; } = "undefined";
        public string Humidity { get; set; } = "undefined";
        public string Feels_like { get; set; } = "undefined";
        public string Pressure { get; set; } = "undefined";
        public required string Kind { get; set; } = "undefined";
        public required string Name { get; set; } = "unknown";
        public required string City { get; set; } = "unknown";
        public required string Country { get; set; } = "unknown";
        public required double Lat { get; set; } = -1;
        public required double Lng { get; set; } = -1;
        public List<InterestingPlaceDto> InterestingPlaces { get; set; } = [];
        public List<string> IdInterestingPlaces { get; set; } = [];
        public Dictionary<string, DescriptionDto> Description { get; set; } = [];

        public void AddInterestingPlaces(InterestingPlaceDto place, string id) {
            InterestingPlaces.Add(place);
            IdInterestingPlaces.Add(id);
        }

        public void AddWeather(string temp, string pressure, string humidity, string feels_like) {
            Temp = temp;
            Pressure = pressure;
            Humidity = humidity;
            Feels_like = feels_like;
        }
    }

    public class DescriptionDto {
        public string Info { get; set; } = "nothing";
        public string Wiki { get; set; } = "nothing";
        public string Kinds { get; set; } = "nothing";
        public string? Image { get; set; }
    }

    public class InterestingPlaceDto {
        public required string Name { get; set; } = "unknown";
        public required double Dist { get; set; } = -1;
    }
}