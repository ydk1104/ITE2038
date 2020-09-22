SELECT name From Pokemon Where id not in (SELECT pid From CatchedPokemon)
ORDER BY name
