SELECT cnt From
(SELECT (SELECT type From Pokemon Where id = pid) as type, COUNT(*) as cnt From CatchedPokemon
GROUP BY type
ORDER BY type) as temp;
