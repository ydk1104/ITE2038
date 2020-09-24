SELECT type From Pokemon where id in (SELECT before_id From Evolution)
GROUP BY type
HAVING COUNT(*)>2
ORDER BY type DESC;
