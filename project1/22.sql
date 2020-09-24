SELECT type, COUNT(*) as cnt From Pokemon
GROUP BY type
ORDER BY cnt, type;
