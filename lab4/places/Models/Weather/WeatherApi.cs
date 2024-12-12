using System.Text.Json;
using System.Threading.Tasks;
using System.Net.Http;
using System;

namespace PlacesApi {
    namespace Weather {
        public class WeatherApi : IPlacesApi {
            public async Task<InformJson>? GetResult(string coordString, HttpClient httpClient) {
                string [] coord = coordString.Split(';');
                var message = await httpClient.GetAsync($"https://api.openweathermap.org/data/2.5/weather?lat={coord[1]}&lon={coord[0]}&appid=8dfba83e7b60159cda4f0733976edb12");
                string response = await message.Content.ReadAsStringAsync();
                return JsonSerializer.Deserialize<WeatherJson>(response);
            }
        }
    }
}