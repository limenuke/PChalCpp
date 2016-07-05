Design Document pChal
Author: Mark Shen

=============================

Describe how you would improve upon your current design.

- Consider using an API to get proper time zone regions based on longitude
  and latitude 
- Allow region definition based on point-defined corners of a region
  (regions are defined by connecting the x points they are composed of) 

=============================

Why did you choose the region definition you did? What other definitions did you consider and what are the pros/cons of each approach?

The current design only returns 8 regions - most of which cover a fault line 
between two plate tectonics maps. The eight regions from four north-south
pairs of regions as divided by 90 degrees longitude in the following ranges:

30 to 120	covers African, Eurasian Indian-Australian and Arabian plate bounds
120 to -150	covers Indian-Australian, Phillipine, Pacific plate bounds
-150 to -60	covers Pacific, Nazca, North & South American plate bounds
-60 to 30	covers North + South American, African & Eurasian plate bounds

=============================

If you wanted to expose this tool as a web service, what approach would you take? What questions would you ask about the product requirements and how would the answers to those questions change your approach?

Approach:
	- Produce a web API for returning regions based on parameters being passed to the API
	- Allow user to define regions by uploading a file that defines a region by its outer points
		- Implement a region checking function to ensure regions don't overlap
	- Allow user to custom define returned data (most impacted, least impacted, most quantity of earthquakes, etc)
	- Store all parsed earthquakes in a day-by-day hashmap where key is the date, value is a pointer ot a list of earthquakes
	that occurred during that date. 
		- Still allows recall fucntions to filter out earthquakes by time of day they occurred. 
		- May store quakes by hours or by week, custom adjust based on data requests

=============================

Product Requirement Questions:
- How quickly must a result be returned?
	-If quickly, need a dedicated stream API as opposed to downloading entire JSON file
- What API calls would be asked for? 
	-Functions must be built for feature requests - for example: return most recently affected
     region
- Region definition may not be point by point - could be country by country or continent by continent, or defined enclosed paths.
	- Build in functions that can interpret various region data and validate it against longitude/latitude data
