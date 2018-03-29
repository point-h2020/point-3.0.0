#!/usr/bin/awk -f

BEGIN {
	i=0
}
{
	fileName = FILENAME

	if ($0 ~ /time/)
	{
		field = NF - 1
		split($field, time, "time=")
		pingTime[i] = time[2]
		i++
	}
}
END {
	for (it = 0; it < i; it++)
	{
		if (it == 0)
		{
			print pingTime[it] > fileName".cleaned"
		}
		else
		{
			print pingTime[it] >> fileName".cleaned"
		}
	}
}
