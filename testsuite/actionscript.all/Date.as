// 
//   Copyright (C) 2005, 2006 Free Software Foundation, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License

// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// Test case for Date ActionScript class
// compile this test case with Ming makeswf, and then
// execute it like this gnash -1 -r 0 -v out.swf

rcsid="$Id: Date.as,v 1.22 2007/04/18 15:03:06 strk Exp $";

#include "check.as"

check (Date);

// Static method should be available even if you haven't asked for a Date object.
//
// We have to specify "000.0" bucause ming fails to parse large integer constants,
// returning 2147483647 instead of the correct value.
check_equals (Date.UTC(2000,0,1).valueOf(), 946684800000.0);

// test the Date constructor exists.
// This specific value is used below to check conversion back to year/mon/day etc
var d = new Date(70,1,2,3,4,5,6);
check (d != undefined);

// test methods' existence
check (d.getDate != undefined);
check (d.getDay != undefined);
check (d.getFullYear != undefined);
check (d.getHours != undefined);
check (d.getMilliseconds != undefined);
check (d.getMinutes != undefined);
check (d.getMonth != undefined);
check (d.getSeconds != undefined);
check (d.getTime != undefined);
check (d.getTimezoneOffset != undefined);
check (d.getUTCDate != undefined);
check (d.getUTCDay != undefined);
check (d.getUTCFullYear != undefined);
check (d.getUTCHours != undefined);
check (d.getUTCMilliseconds != undefined);
check (d.getUTCMinutes != undefined);
check (d.getUTCMonth != undefined);
check (d.getUTCSeconds != undefined);
check (d.getYear != undefined);
check (d.setDate != undefined);
check (d.setFullYear != undefined);
check (d.setHours != undefined);
check (d.setMilliseconds != undefined);
check (d.setMinutes != undefined);
check (d.setMonth != undefined);
check (d.setSeconds != undefined);
check (d.setTime != undefined);
check (d.setUTCDate != undefined);
check (d.setUTCFullYear != undefined);
check (d.setUTCHours != undefined);
check (d.setUTCMilliseconds != undefined);
check (d.setUTCMinutes != undefined);
check (d.setUTCMonth != undefined);
check (d.setUTCSeconds != undefined);
check (d.setYear != undefined);
check (d.toString != undefined);
// UTC is a static method present from v5
check_equals (d.UTC, undefined);
check (Date.UTC != undefined);

#if OUTPUT_VERSION > 6

// From SWF 7 up methods are case-sensitive !
check_equals (d.getdate, undefined);
check_equals (d.getday, undefined);
check_equals (d.getfullYear, undefined);
check_equals (d.gethours, undefined);
check_equals (d.getmilliseconds, undefined);
check_equals (d.getminutes, undefined);
check_equals (d.getmonth, undefined);
check_equals (d.getseconds, undefined);
check_equals (d.gettime, undefined);
check_equals (d.gettimezoneOffset, undefined);
check_equals (d.getUTCdate, undefined);
check_equals (d.getUTCday, undefined);
check_equals (d.getUTCfullYear, undefined);
check_equals (d.getUTChours, undefined);
check_equals (d.getUTCmilliseconds, undefined);
check_equals (d.getUTCminutes, undefined);
check_equals (d.getUTCmonth, undefined);
check_equals (d.getUTCseconds, undefined);
check_equals (d.getyear, undefined);
check_equals (d.setdate, undefined);
check_equals (d.setfullYear, undefined);
check_equals (d.sethours, undefined);
check_equals (d.setmilliseconds, undefined);
check_equals (d.setminutes, undefined);
check_equals (d.setmonth, undefined);
check_equals (d.setseconds, undefined);
check_equals (d.settime, undefined);
check_equals (d.setUTCdate, undefined);
check_equals (d.setUTCfullYear, undefined);
check_equals (d.setUTChours, undefined);
check_equals (d.setUTCmilliseconds, undefined);
check_equals (d.setUTCminutes, undefined);
check_equals (d.setUTCmonth, undefined);
check_equals (d.setUTCseconds, undefined);
check_equals (d.setyear, undefined);
check_equals (d.tostring, undefined);
check_equals (Date.utc, undefined);

#endif // OUTPUT_VERSION > 6

// Some values we will use to test things
    var zero = 0.0;
    var plusinfinity = 1.0/zero;
    var minusinfinity = -1.0/zero;
    var notanumber = zero/zero;

// Check Date constructor in its many forms,
// (also uses valueOf() and toString() methods)

// Constructor with no args sets current localtime
    var d = new Date();
	check (d != undefined);
	// Check it's a valid number after 1 April 2007
	check (d.valueOf() > 1175385600000.0)
	// and before Jan 1 2037 00:00:00
	check (d.valueOf() < 2114380800000.0)

// Constructor with first arg == undefined also sets current localtime
    var d2 = new Date(undefined);
	check (d2 != undefined);
	check (d2.valueOf() >= d.valueOf());
// that shouldn't have taken more than five seconds!
	check (d2.valueOf() < d.valueOf() + 5000);
    delete d2;

// One numeric argument sets milliseconds since 1970 UTC
    delete d; var d = new Date(0);
	// Check UTC "get" methods too
	check_equals(d.valueOf(), 0);
	check_equals(d.getTime(), 0);
	check_equals(d.getUTCFullYear(), 1970);
	check_equals(d.getUTCMonth(), 0);
	check_equals(d.getUTCDate(), 1);
	check_equals(d.getUTCDay(), 4);	// It was a Thursday
	check_equals(d.getUTCHours(), 0);
	check_equals(d.getUTCMinutes(), 0);
	check_equals(d.getUTCSeconds(), 0);
	check_equals(d.getUTCMilliseconds(), 0);
// Check other convertible types
// Booleans convert to 0 and 1
    var foo = true; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), 1);
    foo = false; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), 0);
// Numeric strings
    foo = "12345"; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), 12345.0);
    foo = "12345.0"; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), 12345.0);
    foo = "12345.5"; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), 12345.5);	// Sets fractions of msec ok?
    foo = "-12345"; delete d; var d = new Date(foo);
	check_equals(d.valueOf(), -12345.0);
// Bad numeric values
	// NAN
    delete d; var d = new Date(notanumber);
	check_equals(d.valueOf().toString(), "NaN");
	check_equals(d.toString(), "Invalid Date");
	// Infinity
    delete d; var d = new Date(plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
	check_equals(d.toString(), "Invalid Date");
	// -Infinity
    delete d; var d = new Date(minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
	check_equals(d.toString(), "Invalid Date");
// Bogus values: non-numeric strings
	foo = "bones"; delete d; var d = new Date(foo);
	check_equals(d.valueOf().toString(), "NaN");
	foo = "1234X"; delete d; var d = new Date(foo);
	check_equals(d.valueOf().toString(), "NaN");
// Bogus types: a function
	foo = d.valueOf; var d2 = new Date(foo);
	check_equals(d2.valueOf().toString(), "NaN");
	delete d2;

// Constructor with two numeric args means year and month in localtime.
// Now we check the localtime decoding methods too.
// Negative year means <1900; 0-99 means 1900-1999; 100- means 100-)
// Month is 0-11. month>11 increments year; month<0 decrements year.
    delete d; var d = new Date(70,0);	// 1 Jan 1970 00:00:00 localtime
	check_equals(d.getYear(), 70);
	check_equals(d.getFullYear(), 1970);
	check_equals(d.getMonth(), 0);
	check_equals(d.getDate(), 1);
	check_equals(d.getDay(), 4);	// It was a Thursday
	check_equals(d.getHours(), 0);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 0);
// Check four-figure version - should be the same.
    var d2 = new Date(1970,0); check_equals(d.valueOf(), d2.valueOf());
// Check four-figure version and non-zero month
    delete d; var d = new Date(2000,3);	// 1 April 2000 00:00:00 localtime
	check_equals(d.getYear(), 100);
	check_equals(d.getFullYear(), 2000);
	check_equals(d.getMonth(), 3);
	check_equals(d.getDate(), 1);
	check_equals(d.getDay(), 6);	// It was a Saturday
	check_equals(d.getHours(), 0);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 0);
// Check month overflow/underflow
    delete d; var d = new Date(2000,12);
	check_equals(d.getFullYear(), 2001);
	check_equals(d.getMonth(), 0);
    delete d; var d = new Date(2000,-18);
	check_equals(d.getFullYear(), 1998);
	check_equals(d.getMonth(), 6);
// Bad numeric value handling: year is an invalid number with >1 arg
// The commercial player for these first three cases gives
// -6.77681005679712e+19  Tue Jan -719527 00:00:00 GMT+0000
// but that doesn't seem worth emulating...
    delete d; var d = new Date(notanumber,0);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(plusinfinity,0);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(minusinfinity,0);
	check_equals(d.valueOf().toString(), "-Infinity");
// Bad numeric value handling: month is an invalid number
    delete d; var d = new Date(0,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(0,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(0,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");

// Constructor with three numeric args means year month day-of-month
    delete d; var d = new Date(2000,0,1); // 1 Jan 2000 00:00:00 localtime
	check_equals(d.getFullYear(), 2000);
	check_equals(d.getMonth(), 0);
	check_equals(d.getDate(), 1);
// Check day-of-month overflow/underflow
    delete d; var d = new Date(2000,0,32); // 32 Jan -> 1 Feb
	check_equals(d.getFullYear(), 2000);
	check_equals(d.getMonth(), 1);
	check_equals(d.getDate(), 1);
    delete d; var d = new Date(2000,1,0); // 0 Feb -> 31 Jan
	check_equals(d.getFullYear(), 2000);
	check_equals(d.getMonth(), 0);
	check_equals(d.getDate(), 31);
    delete d; var d = new Date(2000,0,-6); // -6 Jan 2000 -> 25 Dec 1999
	check_equals(d.getFullYear(), 1999);
	check_equals(d.getMonth(), 11);
	check_equals(d.getDate(), 25);
// Bad numeric value handling when day-of-month is an invalid number
// A bad month always returns NaN but a bad d-o-m returns the infinities.
    delete d; var d = new Date(2000,0,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(2000,0,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(2000,0,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
    // Check bad string value
    foo = "bones"; delete d; var d = new Date(2000,0,foo);
	check_equals(d.valueOf().toString(), "NaN");

// Constructor with four numeric args means year month day-of-month hour
    delete d; var d = new Date(2000,0,1,12);
	check_equals(d.getHours(), 12);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 0);
    // Check that fractional parts of hours are ignored
    delete d; var d = new Date(2000,0,1,12.5);
	check_equals(d.getHours(), 12);
	check_equals(d.getMinutes(), 0);
    // Check hours overflow/underflow
    delete d; var d = new Date(2000,0,1,25);
	check_equals(d.getDate(), 2);
	check_equals(d.getHours(), 1);
    // Bad hours, like bad d-o-m, return infinites.
    delete d; var d = new Date(2000,0,1,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(2000,0,1,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(2000,0,1,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
    // Check bad string value
    foo = "bones"; delete d; var d = new Date(2000,0,1,foo);
	check_equals(d.valueOf().toString(), "NaN");

// Constructor with five numeric args means year month day-of-month hour min
    delete d; var d = new Date(2000,0,1,12,30);
	check_equals(d.getHours(), 12);
	check_equals(d.getMinutes(), 30);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 0);
    // Check minute overflow/underflow
    delete d; var d = new Date(2000,0,1,12,70);
	check_equals(d.getHours(), 13);
	check_equals(d.getMinutes(), 10);
	check_equals(d.getSeconds(), 0);
    delete d; var d = new Date(2000,0,1,12,-120);
	check_equals(d.getHours(), 10);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
    // Infinite minutes return infinites.
    delete d; var d = new Date(2000,0,1,0,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(2000,0,1,0,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(2000,0,1,0,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
    // Check bad string value
    foo = "bones"; delete d; var d = new Date(2000,0,1,0,foo);
	check_equals(d.valueOf().toString(), "NaN");

// Constructor with six numeric args means year month d-of-m hour min sec
// Check UTC seconds here too since it should be the same.
    delete d; var d = new Date(2000,0,1,0,0,45);
	check_equals(d.getHours(), 0);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 45);
	check_equals(d.getUTCSeconds(), 45);
	check_equals(d.getMilliseconds(), 0);
    // Check second overflow/underflow
    delete d; var d = new Date(2000,0,1,12,0,70);
	check_equals(d.getHours(), 12);
	check_equals(d.getMinutes(), 1);
	check_equals(d.getSeconds(), 10);
    delete d; var d = new Date(2000,0,1,12,0,-120);
	check_equals(d.getHours(), 11);
	check_equals(d.getMinutes(), 58);
	check_equals(d.getSeconds(), 0);
    // Infinite seconds return infinites.
    delete d; var d = new Date(2000,0,1,0,0,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(2000,0,1,0,0,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(2000,0,1,0,0,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
    // Check bad string value
    foo = "bones"; delete d; var d = new Date(2000,0,1,0,0,foo);
	check_equals(d.valueOf().toString(), "NaN");

// Constructor with seven numeric args means year month dom hour min sec msec
// Check UTC milliseconds here too since it should be the same.
    delete d; var d = new Date(2000,0,1,0,0,0,500);
	check_equals(d.getHours(), 0);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 500);
	check_equals(d.getUTCMilliseconds(), 500);
    // Fractions of milliseconds are ignored here
    delete d; var d = new Date(2000,0,1,0,0,0,500.5);
	check_equals(d.getMilliseconds(), 500.0);
    // Check millisecond overflow/underflow
    delete d; var d = new Date(2000,0,1,12,0,0,1000);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 1);
	check_equals(d.getMilliseconds(), 0);
    delete d; var d = new Date(2000,0,1,12,0,0,-120000);
	check_equals(d.getHours(), 11);
	check_equals(d.getMinutes(), 58);
	check_equals(d.getSeconds(), 0);
    // Infinite milliseconds return infinites.
    delete d; var d = new Date(2000,0,1,0,0,0,notanumber);
	check_equals(d.valueOf().toString(), "NaN");
    delete d; var d = new Date(2000,0,1,0,0,0,plusinfinity);
	check_equals(d.valueOf().toString(), "Infinity");
    delete d; var d = new Date(2000,0,1,0,0,0,minusinfinity);
	check_equals(d.valueOf().toString(), "-Infinity");
    // Check bad string value
    foo = "bones"; delete d; var d = new Date(2000,0,1,0,0,0,foo);
	check_equals(d.valueOf().toString(), "NaN");
    // Finally, check that a millisecond is enough to overflow/underflow a year
    delete d; var d = new Date(1999,11,31,23,59,59,1001);
	check_equals(d.getFullYear(), 2000);
	check_equals(d.getMonth(), 0);
	check_equals(d.getDate(), 1);
	check_equals(d.getMinutes(), 0);
	check_equals(d.getSeconds(), 0);
	check_equals(d.getMilliseconds(), 1);
    delete d; var d = new Date(2000,0,1,0,0,0,-1);
	check_equals(d.getFullYear(), 1999);
	check_equals(d.getMonth(), 11);
	check_equals(d.getDate(), 31);
	check_equals(d.getMinutes(), 59);
	check_equals(d.getSeconds(), 59);
	check_equals(d.getMilliseconds(), 999);
// If a mixture of infinities and/or NaNs are present the result is NaN.
    delete d; var d = new Date(2000,0,1,plusinfinity,minusinfinity,0,0);
	check_equals(d.valueOf().toString(), "NaN");

// It's hard to test TimezoneOffset because the values will be different
// depending upon where geographically you run the tests.
// We do what we can without knowing where we are!

// Set midnight local time, adjust for tzoffset
// and this should give us midnight UTC.
//
// If we are in GMT+1 then TimezoneOffset is -60.
// If we set midnight localtime in the GMT+1 zone,
// that is 23:00 the day before in UTC (because in GMT+1 clock times happen
// an hour earlier than they do in "real" time).
// Thus to set UTC to midnight we need to subtract the TimezoneOffset.
delete d;
var d = new Date(2000, 0, 1, 0, 0, 0, 0);
d.setTime(d.getTime() - (60000 * d.getTimezoneOffset()));
check_equals (d.getUTCHours(), 0);

// Try the same thing in July to get one with DST and one without
d = new Date(2000, 6, 1, 0, 0, 0, 0);
d.setTime(d.getTime() - (60000 * d.getTimezoneOffset()));
check_equals (d.getUTCHours(), 0);


// Test behaviour when you set the time during DST then change
// to a non-DST date.
// setUTCHours should preserve the time of day in UTC;
// setHours should preserve the time of day in localtime.
//
// We assume that January/December and June/July will have different DST values

trace ("Testing hour when setting date into/out of DST");
d.setUTCFullYear(2000, 0, 1);
d.setUTCHours(0, 0, 0, 0);
d.setUTCMonth(6);
check_equals (d.getUTCHours(), 0);

d.setUTCFullYear(2000, 6, 1);
d.setUTCHours(0, 0, 0, 0);
d.setUTCMonth(11);
check_equals (d.getUTCHours(), 0);

d.setFullYear(2000, 0, 1);
d.setHours(0, 0, 0, 0);
d.setMonth(6);
check_equals (d.getHours(), 0);

d.setFullYear(2000, 6, 1);
d.setHours(0, 0, 0, 0);
d.setMonth(11);
check_equals (d.getHours(), 0);

// It's not easy to test the toString() code here cos we cannot find out from
// within AS whether DST is in effect or not.

check_equals (Date.UTC(1970,0), 0);
check_equals (Date.UTC(70,0), 0);

// Check that Date.UTC gives the same as setUTC*, which we tested above.
// Test two dates: one in DST and one not.
d.setUTCFullYear(2000, 0, 1);
d.setUTCHours(0, 0, 0, 0);
check (Date.UTC(2000,0,1,0,0,0,0) == d.valueOf());
d.setUTCFullYear(2000, 6, 1);
d.setUTCHours(0, 0, 0, 0);
check (Date.UTC(2000,6,1,0,0,0,0) == d.valueOf());
