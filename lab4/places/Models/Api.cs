using System;
using System.Net.Http;
using System.Threading.Tasks;

namespace PlacesApi {
    public interface IPlacesApi {
        public Task<InformJson>? GetResult(String information, HttpClient httpClient);
    }
}