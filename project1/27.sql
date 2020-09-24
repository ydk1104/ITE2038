SELECT name, MAX(level) From CatchedPokemon, Trainer
WHERE owner_id = Trainer.id
GROUP BY owner_id
HAVING COUNT(*) > 3
ORDER BY name;
