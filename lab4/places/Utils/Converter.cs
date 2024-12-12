using System;

namespace Utils;

public class Converter
{
    private static readonly double temperatureConstant = 273.15;
    private static readonly double pressureConstant = 0.75;

    public static double TemperatureInKelvinToCelsius(double kelvinTemp)
    {
        return Math.Round(kelvinTemp - temperatureConstant);
    }

    public static double PressureInHPaToMilimetersOfMercury(double pressureInHPa)
    {
        return Math.Round(pressureInHPa * pressureConstant);
    }
}