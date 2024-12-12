namespace PlacesApi {
    namespace Location {
        public class LocationJson : InformJson {
            public Place[]? hits { get; set; }
        }

        public class Place {
            public string name { get; set; } = "unknown";
            public string country { get; set; } = "unknown";
            public string city { get; set; } = "unknown";
            public string postcode { get; set; } = "-1";
            public Point point { get; set; } = new Point();
            public string osm_value { get; set; } = "nothing";
        }

        public class Point {
            public double lng { get; set; } = -1;
            public double lat { get; set; } = -1;
        }
    }
}