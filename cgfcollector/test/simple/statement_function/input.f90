program main
    implicit none

    integer :: a, b, c
    integer :: res1, res2, res3
    integer :: mul, add, mix

    mul(a,b) = a * b
    add(a,b) = a + b
    mix(a,b,c) = mul(a,b) + add(b,c)

    res1 = mul(5,2)
    res2 = add(3,4)

    res3 = mix(2,3,4)

    print *, "mul:", res1
    print *, "add:", res2
    print *, "mix:", res3

    call inner_scope()

contains

    subroutine inner_scope()
        implicit none

        integer :: a, b, res, mul

        mul(a,b) = a * b + 1

        res = mul(2,3)

        print *, "inner mul:", res

        call use_arg(res)
    end subroutine inner_scope


    subroutine use_arg(x)
        implicit none

        integer, intent(in) :: x
        integer :: a, b, subaddx

        subaddx(a,b) = a - b + x

        print *, "use_arg:", subaddx(10,3)
    end subroutine use_arg

end program main
