namespace PlacesApi
{
    namespace Description
    {
        public class DescriptionJson : InformJson
        {
            public string xid { get; set; } = "-1";
            public string name { get; set; } = "unknown";
            public string kinds { get; set; } = "undefined";
            public string? image { get; set; }
            public Info info { get; set; } = new Info();
            public Wiki wikipedia_extracts { get; set; } = new Wiki();
        }

        public class Info
        {
            public string descr { get; set; } = "nothing";
        }


        public class Wiki
        {
            public string text { get; set; } = "nothing";
        }
    }
}