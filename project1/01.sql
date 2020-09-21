SELECT name from Trainer where id in
(SELECT owner_id FROM CatchedPokemon
GROUP BY owner_id
HAVING COUNT(*) > 2)
ORDER BY (
  SELECT COUNT(*) FROM CatchedPokemon WHERE CatchedPokemon.owner_id=Trainer.id
) DESC
