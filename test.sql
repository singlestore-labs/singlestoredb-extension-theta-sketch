create temporary table sketch_input (id1 int, id2 int);
insert into sketch_input values
  (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);

create table IF NOT EXISTS sketch_intermediate (sketch1 blob, sketch2 blob);

insert into sketch_intermediate select theta_sketch(id1), theta_sketch(id2) from sketch_input;

select
    sketch_estimate(sketch1),
    sketch_estimate(sketch2),
    sketch_estimate(sketch_union(sketch1, sketch2)),
    sketch_estimate((sketch_intersect(sketch1, sketch2))),
    sketch_estimate(sketch_anotb(sketch1, sketch2)),
    sketch_estimate(sketch_anotb(sketch2, sketch1))
from sketch_intermediate;

----
--V2

create table IF NOT EXISTS sketch_input (id1 int, id2 int);
insert into sketch_input values
                             (1, 2), (2, 4), (3, 6), (4, 8), (5, 10), (6, 12), (7, 14), (8, 16), (9, 18), (10, 20);
select sketch_print(sketch_union(theta_sketch(id1), theta_sketch(id2))) from sketch_input;