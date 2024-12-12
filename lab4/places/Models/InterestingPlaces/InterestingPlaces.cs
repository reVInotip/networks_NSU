using System.Text.Json;
using System.Threading.Tasks;
using System.Net.Http;
using System;

namespace PlacesApi
{
    namespace InterestingPlaces
    {
        public class InterestingPlacesApi : IPlacesApi
        {
            public async Task<InformJson>? GetResult(string coordString, HttpClient httpClient)
            {
                string[] coord = coordString.Split(';');
                var message = await httpClient.GetAsync($"https://api.opentripmap.com/0.1/ru/places/radius?radius=1000&lon={coord[0].Replace(',', '.')}&lat={coord[1].Replace(',', '.')}&apikey=5ae2e3f221c38a28845f05b6525262d2e9501766df7dc543333148a9");
                string response = await message.Content.ReadAsStringAsync();
                return JsonSerializer.Deserialize<InterestingPlacesJson>(response);
            }
        }
    }
}