SELECT name From Pokemon
Where type = 'Grass' and id in (SELECT before_id From Evolution)
