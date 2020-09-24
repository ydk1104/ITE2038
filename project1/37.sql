SELECT name, S From
(SELECT SUM(level) as S, owner_id From CatchedPokemon
GROUP BY owner_id
Having S =
(SELECT MAX(S) From (SELECT SUM(level) as S From CatchedPokemon
GROUP BY owner_id) as T)) as T, Trainer
Where owner_id = id;
