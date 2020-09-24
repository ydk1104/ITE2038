SELECT name From Trainer,
(SELECT owner_id, nickname From CatchedPokemon Where pid in
(SELECT after_id From Evolution Where after_id not in
(SELECT before_id From Evolution))) As Temp
Where Trainer.id = Temp.owner_id
ORDER BY name;
