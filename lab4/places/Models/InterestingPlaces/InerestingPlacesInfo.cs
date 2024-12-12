namespace PlacesApi
{
    namespace InterestingPlaces
    {
        public class InterestingPlacesJson : InformJson
        {
            public InterestingPlace[]? features { get; set; }
        }

        public class InterestingPlace
        {
            public string id { get; set; } = "-1";
            public Properties properties { get; set; } = new Properties();
            public Point? point { get; set; }
        }

        public class Properties
        {
            public string xid { get; set; } = "-1";
            public string name { get; set; } = "unknown";
            public double dist { get; set; } = -1;
            public string kinds { get; set; } = "nothing";
        }

        public class Point
        {
            public double lon { get; set; } = -1;
            public double lat { get; set; } = -1;
        }
    }
}