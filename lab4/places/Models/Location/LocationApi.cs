using System.Text.Json;
using System.Threading.Tasks;
using System.Net.Http;
using System;

namespace PlacesApi
{
    namespace Location
    {
        public class LocationApi : IPlacesApi
        {
            public async Task<InformJson>? GetResult(string name, HttpClient httpClient)
            {
                var message = await httpClient.GetAsync($"https://graphhopper.com/api/1/geocode?q={name}&locale=ru&limit=10&reverse=false&debug=false&provider=default&key=db9c1a11-bc76-473f-bc69-e651ca81c492");
                if (message is null) return null;

                string response = await message.Content.ReadAsStringAsync();
                return JsonSerializer.Deserialize<LocationJson>(response);
            }
        }
    }
}