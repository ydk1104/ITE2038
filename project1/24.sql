(SELECT name, avg_level From
(SELECT AVG(level) As avg_level, (SELECT hometown From Trainer where id = owner_id) As Town From CatchedPokemon
GROUP BY town) As temp RIGHT JOIN City ON temp.Town = City.name)
ORDER BY avg_level;
