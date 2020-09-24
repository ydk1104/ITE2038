SELECT AVG(level) from CatchedPokemon where
owner_id in (SELECT id from Trainer where hometown = 'Sangnok City')
and pid in (SELECT id from Pokemon where type = 'Electric');
