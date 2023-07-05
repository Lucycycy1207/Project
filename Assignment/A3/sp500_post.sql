-- Create the tables for the S&P 500 database.
-- They contain information about the companies 
-- in the S&P 500 stock market index
-- during some interval of time in 2014-2015.
-- https://en.wikipedia.org/wiki/S%26P_500 

create table history
(
	symbol text,
	day date,
	open numeric,
	high numeric,
	low numeric,
	close numeric,
	volume integer,
	adjclose numeric
);

create table sp500
(
	symbol text,
	security text,
	sector text,
	subindustry text,
	address text,
	state text
);

-- Populate the tables
insert into history select * from bob.history;
insert into sp500 select * from bob.sp500;

-- Familiarize yourself with the tables.
select * from history;
select * from sp500;


-- Exercise 1 (3 pts)

-- 1. (1 pts) Find the number of companies for each state, sort descending by the number.

select count(security) as num, state from sp500 group by state order by num desc;



-- 2. (1 pts) Find the number of companies for each sector, sort descending by the number.

select count(security) as num, sector from sp500 group by sector order by num desc;


-- 3. (1 pts) Order the days of the week by their average volatility.
-- Sort descending by the average volatility. 
-- Use 100*abs(high-low)/low to measure daily volatility.

select
       extract(dow from day) as dayofweek,

       avg(100*abs(high-low)/low) as avgvol
from history
group by dayofweek
order by 2 desc;




-- Exercise 2 (4 pts)

-- 1. (2 pts) Find for each symbol and day the pct change from the previous business day.
-- Order descending by pct change. Use adjclose.

--pct change from the previous business day is:
--100*(adjclose-adjclose_prev)/adjclose_prev

--adjclose_prev can be found using:
--LAG(adjclose,1) OVER (...) AS adjclose_prev

create view t1 as (
select symbol, day, adjclose from history
group by symbol, day, adjclose order by symbol, day);

create view t2 as (
select symbol, day, adjclose,
       lag(adjclose,1) over (order by symbol,day) as adjclose_prev
from t1);

select symbol, day, 100*(adjclose-adjclose_prev)/adjclose_prev as pctchange from t2 order by pctchange;

drop view t2;
drop view t1;

-- 2. (2 pts)
-- Many traders believe in buying stocks in uptrend
-- in order to maximize their chance of profit. 
-- Let us check this strategy.
-- Find for each symbol on Oct 1, 2015 
-- the pct change 20 trading days earlier and 20 trading days later.
-- Order descending by pct change with respect to 20 trading days earlier.
-- Use adjclose.

-- Expected result
--symbol,pct_change,pct_change2
--TE,26.0661102331371252,3.0406725557250169
--TAP,24.6107784431137725,5.1057184046131667
--CVC,24.4688922610015175,-0.67052727826882048156
--...

create view t1 as (
select symbol, day, adjclose from history
group by symbol, day, adjclose order by symbol, day);

create view t2 as (
select symbol, day, adjclose,
       lag(adjclose,20) over (order by symbol,day) as adjclose_prev,
       lead(adjclose,20) over (order by symbol,day) as adjclose_later
from t1);

select symbol,
       100*(adjclose-adjclose_prev)/adjclose_prev as pct_change,
       100*(adjclose_later-adjclose)/adjclose as pct_change2 from t2 where day='2015-10-01'
order by pct_change desc;

drop view t2;
drop view t1;

-- Exercise 3 (3 pts)
-- Find the top 10 symbols with respect to their average money volume AVG(volume*adjclose).
-- Use round(..., -8) on the average money volume.
-- Give three versions of your query, using ROW_NUMBER(), RANK(), and DENSE_RANK().
create view t1 as (
select symbol,
       round(avg(volume * adjclose),-8) as avg_money_vol
from history group by symbol order by avg_money_vol desc limit 10);

select row_number() over(order by symbol asc) as Row,
       symbol,avg_money_vol
from t1;

select rank() over(order by avg_money_vol desc) as rank,
       symbol, avg_money_vol
from t1;

select dense_rank() over(order by avg_money_vol desc) as rank,
       symbol, avg_money_vol
from t1;

drop view t1;

