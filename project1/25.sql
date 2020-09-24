SELECT DISTINCT name From Pokemon where id in
(SELECT pid From CatchedPokemon,Trainer where owner_id = Trainer.id && Trainer.hometown = 'Sangnok City') and id in
(SELECT pid From CatchedPokemon,Trainer where owner_id = Trainer.id && Trainer.hometown = 'Brown City')
ORDER BY name;
