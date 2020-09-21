SELECT name from Pokemon where type in
(SELECT type from Pokemon GROUP BY type HAVING COUNT(type)>=
(SELECT COUNT(type) from Pokemon
GROUP BY type
ORDER BY COUNT(type) DESC LIMIT 1,1))
ORDER BY name
