namespace PlacesApi
{
    namespace Weather
    {
        public class WeatherJson : InformJson
        {
            public Weather[]? weather { get; set; }
            public State main { get; set; } = new State();
            public int visibility { get; set; } = -1;
            public Wind wind { get; set; } = new Wind();
            public Clouds clouds { get; set; } = new Clouds();
            public long dt { get; set; } = -1;
            public int timezone { get; set; } = -1;
            public long id { get; set; } = -1;
            public string name { get; set; } = "unknown";
        }

        public class Wind
        {
            public double speed { get; set; }
            public int deg { get; set; }
            public double gust { get; set; }
        }

        public class Weather
        {
            public long id { get; set; }
            public string name { get; set; } = "unknown";
            public string description { get; set; } = "nothing";
            public string icon { get; set; } = "nothing";
        }


        public class State
        {
            public double temp { get; set; }
            public double feels_like { get; set; }
            public int pressure { get; set; }
            public int humidity { get; set; }
        }

        public class Clouds
        {
            public int all { get; set; }
        }
    }
}